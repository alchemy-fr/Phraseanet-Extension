#ifndef PHP_PHRASEA2_H
#define PHP_PHRASEA2_H 1

#include "phrasea_engine/cache.h"	// define CACHE_SESSION

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(phrasea2)
	SQLCONN *epublisher;
	CACHE_SESSION *global_session;
 	char tempPath[1024];				// tmp dir
ZEND_END_MODULE_GLOBALS(phrasea2)

#ifdef ZTS
#define PHRASEA2_G(v) TSRMG(phrasea2_globals_id, zend_phrasea2_globals *, v)
#else
#define PHRASEA2_G(v) (phrasea2_globals.v)
#endif

PHP_MINIT_FUNCTION(phrasea2);
PHP_MSHUTDOWN_FUNCTION(phrasea2);
PHP_RINIT_FUNCTION(phrasea2);
PHP_RSHUTDOWN_FUNCTION(phrasea2);
PHP_MINFO_FUNCTION(phrasea2);

PHP_FUNCTION(phrasea_create_session);
PHP_FUNCTION(phrasea_open_session);
PHP_FUNCTION(phrasea_clear_cache);
PHP_FUNCTION(phrasea_close_session);
PHP_FUNCTION(phrasea_query2); 
PHP_FUNCTION(phrasea_fetch_results);
PHP_FUNCTION(phrasea_conn);
PHP_FUNCTION(phrasea_info);

extern zend_module_entry phrasea2_module_entry;
#define phpext_phrasea2_ptr &phrasea2_module_entry


// extern ZEND_DECLARE_MODULE_GLOBALS(phrasea2)
ZEND_EXTERN_MODULE_GLOBALS(phrasea2)
	
#endif	/* PHP_PHRASEA2_H */
