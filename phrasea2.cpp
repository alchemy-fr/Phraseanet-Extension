// #include <sys/types.h>
// #include <sys/timeb.h>
// #include <math.h>
#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#pragma warning(push)
#pragma warning(disable:4005)		// avoid warning about double def of _WIN32_WINNT
#include "php.h"
#include "php_ini.h"
#pragma warning(pop)
#include "ext/standard/info.h"



#ifdef PHP_WIN32
#include <winsock2.h>
#define signal(a, b) NULL
#elif defined(NETWARE)
#include <sys/socket.h>
#define signal(a, b) NULL
#else
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <mysql.h>


#include "_VERSION.h"

#include "phrasea_engine/base_header.h"
#include "phrasea_engine/sql.h"

#include "php_phrasea2.h"

#include "phrasea_engine/trace_memory.h"
#include "phrasea_engine/ftrace.h"

ZEND_DECLARE_MODULE_GLOBALS(phrasea2)

// -----------------------------------------------------------------------------
// option -Wno-write-strings to gcc prevents warnings on this section
static zend_function_entry phrasea2_functions[] = {
	PHP_FE(phrasea_info, NULL)
	PHP_FE(phrasea_conn, NULL)
	PHP_FE(phrasea_create_session, NULL)
	PHP_FE(phrasea_open_session, NULL)
	PHP_FE(phrasea_clear_cache, NULL)
	PHP_FE(phrasea_close_session, NULL)
	PHP_FE(phrasea_query2, NULL)
	PHP_FE(phrasea_fetch_results, NULL)
	{
		NULL, NULL, NULL
	} /* Must be the last line in phrasea2_functions[] */
};
// -----------------------------------------------------------------------------

#pragma push
#pragma GCC diagnostic ignored "-Wwrite-strings"

zend_module_entry phrasea2_module_entry = {
	STANDARD_MODULE_HEADER,
	"phrasea2",
	phrasea2_functions,
	PHP_MINIT(phrasea2),
	PHP_MSHUTDOWN(phrasea2),
	PHP_RINIT(phrasea2),
	PHP_RSHUTDOWN(phrasea2),
	PHP_MINFO(phrasea2),
	"0.1",
	STANDARD_MODULE_PROPERTIES
};	// pragma avoids "warning: deprecated conversion from string constant to ‘char*’" on this line

#pragma pop

#ifdef COMPILE_DL_PHRASEA2

ZEND_GET_MODULE(phrasea2)
#endif



PHP_INI_BEGIN()
PHP_INI_END()


static void php_phrasea2_init_globals(zend_phrasea2_globals *phrasea2_globals)
{
	phrasea2_globals->epublisher = NULL;
	phrasea2_globals->global_session = NULL;
}
// -----------------------------------------------------------------------------
// option -Wno-write-strings to gcc prevents warnings on this section

PHP_MINIT_FUNCTION(phrasea2)
{
	ZEND_INIT_MODULE_GLOBALS(phrasea2, php_phrasea2_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("PHRASEA_OP_OR", PHRASEA_OP_OR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_AND", PHRASEA_OP_AND, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_KW_ALL", PHRASEA_KW_ALL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_KW_LAST", PHRASEA_KW_LAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_KW_FIRST", PHRASEA_KW_FIRST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_EXCEPT", PHRASEA_OP_EXCEPT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_NEAR", PHRASEA_OP_NEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_BEFORE", PHRASEA_OP_BEFORE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_AFTER", PHRASEA_OP_AFTER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_IN", PHRASEA_OP_IN, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PHRASEA_OP_COLON", PHRASEA_OP_COLON, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_EQUAL", PHRASEA_OP_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_NOTEQU", PHRASEA_OP_NOTEQU, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_GT", PHRASEA_OP_GT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_LT", PHRASEA_OP_LT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_GEQT", PHRASEA_OP_GEQT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_OP_LEQT", PHRASEA_OP_LEQT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PHRASEA_KEYLIST", PHRASEA_KEYLIST, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PHRASEA_MULTIDOC_DOCONLY", PHRASEA_MULTIDOC_DOCONLY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_MULTIDOC_REGONLY", PHRASEA_MULTIDOC_REGONLY, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PHRASEA_ORDER_DESC", PHRASEA_ORDER_DESC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_ORDER_ASC", PHRASEA_ORDER_ASC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHRASEA_ORDER_ASK", PHRASEA_ORDER_ASK, CONST_CS | CONST_PERSISTENT);


#ifdef ZTS
#if MYSQL_VERSION_ID >= 40000
	// mysql_thread_init();
#endif
#endif
	mysql_library_init(0, NULL, NULL);
//	char *bug1 = (char*)(malloc(666));
//	char *bug2 = (char*)(EMALLOC(333));
	return SUCCESS;
}
// -----------------------------------------------------------------------------

PHP_RINIT_FUNCTION(phrasea2)
{
	PHRASEA2_G(global_session) = NULL;
	PHRASEA2_G(epublisher) = NULL;

#ifdef PHP_WIN32
	DWORD tempPathLen;
	tempPathLen = GetTempPath(1023, PHRASEA2_G(tempPath));
	if(tempPathLen > 0 || tempPathLen < 1023)
	{
		//		tempPathBuffer[tempPathLen] = '\0';
	}
	else
	{
		PHRASEA2_G(tempPath)[0] = '\\';
		PHRASEA2_G(tempPath)[1] = '\0';
	}
#else
	strcpy(PHRASEA2_G(tempPath), "/tmp/");
#endif

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(phrasea2)
{
	if(PHRASEA2_G(global_session))
	{
		delete PHRASEA2_G(global_session);
		PHRASEA2_G(global_session) = NULL;
	}
	if(PHRASEA2_G(epublisher))
	{
		delete PHRASEA2_G(epublisher);
		PHRASEA2_G(epublisher) = NULL;
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(phrasea2)
{
	mysql_library_end();
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(phrasea2)
{
	char buf[1000];
	php_info_print_table_start();
	php_info_print_table_header(2, "phrasea2 support", "enabled");
	php_info_print_table_row(2, "Version", QUOTE(PHDOTVERSION));

	sprintf(buf, "OK ( client info : %s )", mysql_get_client_info());
	php_info_print_table_row(2, "MYSQL support", buf);
	if(mysql_thread_safe())
		php_info_print_table_row(2, "MYSQL thead safe", "YES");
	else
		php_info_print_table_row(2, "MYSQL thead safe", "NO");

//	php_info_print_table_row(2, "NO PostgreSQL support", "");

#ifdef MYSQLENCODE
	php_info_print_table_row(2, "SQL connection charset", QUOTE(MYSQLENCODE));
#else
	php_info_print_table_row(2, "SQL connection charset", "default");
#endif
	FILE *fp_test = NULL;
	char *fname;
	bool test = false;

	int l = strlen(PHRASEA2_G(tempPath))
		+ 9 // "_phrasea."
		+ strlen("fakeukey")
		+ 9 // ".answers."
		+ 33 // zsession
		+ 1; // '\0'

	if((fname = (char *) EMALLOC(l)))
	{
		sprintf(fname, "%s_phrasea.%s.test.%d.bin", PHRASEA2_G(tempPath), "fakeukey", 0);
		if((fp_test = fopen(fname, "ab")))
		{
			fclose(fp_test);
			test = true;
		}

//		php_info_print_table_row(3, "temp DIR", PHRASEA2_G(tempPath), (test ? "OK" : "BAD"));
		php_info_print_table_row(3, "temp DIR", fname, (test ? "OK" : "BAD"));

		EFREE(fname);
	}

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

PHP_FUNCTION(phrasea_info)
{
	if(ZEND_NUM_ARGS() != 0)
		WRONG_PARAM_COUNT; // returns !

	char fname[1000];
	FILE *fp_test;
	SQLCONN *epublisher = NULL;

	array_init(return_value);
	add_assoc_string(return_value, (char *) "version", (char *)QUOTE(PHDOTVERSION), TRUE);
	add_assoc_string(return_value, (char *) "mysql_client", (char *) (mysql_get_client_info()), TRUE);
	add_assoc_bool(return_value, (char *) "mysql_thread_safe", mysql_thread_safe());
	add_assoc_string(return_value, (char *) "temp_dir", PHRASEA2_G(tempPath), TRUE);

	sprintf(fname, "%s_test.bin", PHRASEA2_G(tempPath));
	if((fp_test = fopen(fname, "ab")))
	{
		fclose(fp_test);
		remove(fname);
		add_assoc_bool(return_value, (char *) "temp_writable", true);
	}
	else
	{
		add_assoc_bool(return_value, (char *) "temp_writable", false);
	}

	epublisher = PHRASEA2_G(epublisher);
	if(epublisher)
	{
		if(epublisher->connect())
		{
			epublisher->close();
			add_assoc_string(return_value, (char *) "cnx_ukey", epublisher->ukey, TRUE);
		}
		else
		{
			add_assoc_bool(return_value, (char *) "cnx_ukey", FALSE);
		}
	}
	else
	{
		add_assoc_bool(return_value, (char *) "cnx_ukey", FALSE);
	}
}

PHP_FUNCTION(phrasea_conn)
{
	if(ZEND_NUM_ARGS() != 5)
		WRONG_PARAM_COUNT; // returns !

	char *zhost, *zuser, *zpasswd, *zdbname;
	int zhost_len, zuser_len, zpasswd_len, zdbname_len;
	long zport;

	if(zend_parse_parameters(5 TSRMLS_CC, (char *) "slsss"
							, &zhost, &zhost_len
							, &zport
							, &zuser, &zuser_len
							, &zpasswd, &zpasswd_len
							, &zdbname, &zdbname_len) == FAILURE)
	{
		RETURN_FALSE;
	}
	if(zhost_len > 1000)
		zhost[1000] = '\0';

	if(zuser_len > 1000)
		zuser[1000] = '\0';

	if(zpasswd_len > 1000)
		zpasswd[1000] = '\0';

	if(zdbname_len > 1000)
		zdbname[1000] = '\0';

	if(PHRASEA2_G(epublisher))
		delete(PHRASEA2_G(epublisher));

	PHRASEA2_G(epublisher) = new SQLCONN(zhost, (int) zport, zuser, zpasswd, zdbname);
	if(PHRASEA2_G(epublisher)->connect())
	{
		PHRASEA2_G(epublisher)->close();
		RETURN_TRUE;
	}
	else
	{
		delete(PHRASEA2_G(epublisher));
		PHRASEA2_G(epublisher) = NULL;
		RETURN_FALSE;
	}
}



