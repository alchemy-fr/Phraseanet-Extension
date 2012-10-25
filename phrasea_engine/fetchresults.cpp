#include "base_header.h"

#include "../php_phrasea2/php_phrasea2.h"

#include "phrasea_clock_t.h"
#include "sql.h"

#include "ftrace.h"

typedef struct hbal
{
	unsigned long offset;
	bool closing;
}
HBAL;

char THEX[] = "0123456789ABCDEF";


ZEND_FUNCTION(phrasea_fetch_results)
{
	long session, firstanswer, nanswers;
	long getxml = false;
	char *markin = NULL, *markout = NULL;
	int markin_l = 0, markout_l = 0;

	CHRONO time_phpfct;

	startChrono(time_phpfct);
	switch(ZEND_NUM_ARGS())
	{
		case 3:
			if(zend_parse_parameters(3 TSRMLS_CC, (char *) "lll", &session, &firstanswer, &nanswers) == FAILURE)
				RETURN_FALSE;
			break;
		case 4:
			if(zend_parse_parameters(4 TSRMLS_CC, (char *) "lllb", &session, &firstanswer, &nanswers, &getxml) == FAILURE)
				RETURN_FALSE;
			break;
		case 6:
			if(zend_parse_parameters(6 TSRMLS_CC, (char *) "lllbss", &session, &firstanswer, &nanswers, &getxml, &markin, &markin_l, &markout, &markout_l) == FAILURE)
				RETURN_FALSE;
			break;
		default:
			WRONG_PARAM_COUNT;	// returns !
			break;
	}

	SQLCONN *epublisher = PHRASEA2_G(epublisher);

	if(!epublisher)
	{
/*
zend_exception_get_default(TSRMLS_C)
zend_get_error_exception(TSRMLS_D)

spl_ce_LogicException;
spl_ce_BadFunctionCallException;
spl_ce_BadMethodCallException;
spl_ce_DomainException;
spl_ce_InvalidArgumentException;
spl_ce_LengthException;
spl_ce_OutOfRangeException;
spl_ce_RuntimeException;
spl_ce_OutOfBoundsException;
spl_ce_OverflowException;
spl_ce_RangeException;
spl_ce_UnderflowException;
spl_ce_UnexpectedValueException;
*/
		// we need conn !
//		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "No connection set (check that phrasea_conn(...) returned true).", 0 TSRMLS_CC);
//		zend_throw_exception(spl_ce_LogicException, "No connection set (check that phrasea_conn(...) returned true).", 0 TSRMLS_CC);
		RETURN_FALSE;
	}

	// we need the session only if we ask for xml
	if(getxml)
	{
		if(!PHRASEA2_G(global_session) || PHRASEA2_G(global_session)->get_session_id() != session)
		{
			// no session, can't continue
			RETURN_FALSE;
		}
	}

	char tmpstr[1024]; // buffer to format short messages (error messages...)
	CHRONO chrono;

	sprintf(tmpstr, "UPDATE cache SET nact=nact+1, lastaccess=NOW() WHERE session_id=%li", session);
	if(epublisher->query(tmpstr) && (epublisher->affected_rows() == 1))
	{
		SQLRES res(epublisher);
		if(firstanswer < 1)
			firstanswer = 1;

		CACHE_ANSWER *panswer = NULL;
		FILE *fp_answers = NULL;
		unsigned int nanswers_incache = 0;


		char *fname;
		int l = strlen(PHRASEA2_G(tempPath))
			+ 9 // "_phrasea."
			+ strlen(epublisher->ukey)
			+ 9 // ".answers."
			+ 33 // zsession
			+ 1; // '\0'
		if((fname = (char *) EMALLOC(l)))
		{
			sprintf(fname, "%s_phrasea.%s.answers.%ld.bin", PHRASEA2_G(tempPath), epublisher->ukey, session);

			startChrono(chrono);
			if((fp_answers = fopen(fname, "rb")))
			{
				unsigned int answer_index0 = firstanswer - 1;
				if((fseek(fp_answers, answer_index0 * sizeof (CACHE_ANSWER), SEEK_SET)) == 0)
				{
					unsigned int nanswers_toread = (unsigned int) nanswers; // WARNING : long to int
					if(panswer = (CACHE_ANSWER *) EMALLOC(nanswers_toread * sizeof (CACHE_ANSWER)))
					{
						nanswers_incache = fread(panswer, sizeof (CACHE_ANSWER), nanswers_toread, fp_answers);

						CACHE_SPOT *pspot = NULL;
						FILE *fp_spots = NULL;
						unsigned int nspots_incache = 0;
						unsigned int spot_index0 = 0;

						sprintf(fname, "%s_phrasea.%s.spots.%ld.bin", PHRASEA2_G(tempPath), epublisher->ukey, session);

						if((fp_spots = fopen(fname, "rb")))
						{
							spot_index0 = panswer[0].spots_index;
							if(fseek(fp_spots, spot_index0 * sizeof (CACHE_SPOT), SEEK_SET) == 0)
							{
								unsigned int nspots_toread = 0;
								for(unsigned int a = 0; a < nanswers_incache; a++)
									nspots_toread += panswer[a].nspots;

								if(pspot = (CACHE_SPOT *) EMALLOC(nspots_toread * sizeof (CACHE_SPOT)))
								{
									nspots_incache = fread(pspot, sizeof (CACHE_SPOT), nspots_toread, fp_spots);
								}
							}
							fclose(fp_spots);
						}

						array_init(return_value);

						add_assoc_double(return_value, (char *) "time_readCache", stopChrono(chrono));

						char hex_status[16 + 1];
//						unsigned long long llstatus;

						zval *zanswers;
						MAKE_STD_ZVAL(zanswers);
						array_init(zanswers);

						for(unsigned int a = 0; a < nanswers_incache; a++)
						{
							zval *zanswer;
							MAKE_STD_ZVAL(zanswer);
							array_init(zanswer);

							add_assoc_long(zanswer, (char *) "base_id", panswer[a].bid);
							add_assoc_long(zanswer, (char *) "record_id", panswer[a].rid);

							if(getxml)
							{
								startChrono(chrono);
								SQLCONN *conn = PHRASEA2_G(global_session)->connect(panswer[a].bid);
								add_assoc_double(zanswer, (char *) "time_dboxConnect", stopChrono(chrono));

								if(conn)
								{
									SQLRES res(conn);
									sprintf(tmpstr, "SELECT xml, parent_record_id, HEX(status) FROM record WHERE record_id=%i", panswer[a].rid);
									startChrono(chrono);
									if(res.query(tmpstr))
									{
										add_assoc_double(zanswer, (char *) "time_xmlQuery", stopChrono(chrono));
										SQLROW *row;
										char *xml;
										startChrono(chrono);
										if(row = res.fetch_row())
										{
											add_assoc_double(zanswer, (char *) "time_xmlFetch", stopChrono(chrono));

											unsigned long *siz = res.fetch_lengths();
											add_assoc_long(zanswer, (char *) "parent_record_id", atol(row->field(1, "0")));
											memset(hex_status, '0', 17);
											memcpy(hex_status + (16 - siz[2]), row->field(2, "0"), siz[2]);
											add_assoc_stringl(zanswer, (char *) "status", hex_status, 16, TRUE);

											unsigned long xmlsize = siz[0] + 1;

											// count spots
											unsigned long s0 = (panswer[a].spots_index - spot_index0);

											// size the highlighted xml
											unsigned int nspots = panswer[a].nspots;

											if(xml = (char *) EMALLOC(xmlsize + (nspots * (markin_l + markout_l))))
											{
												memcpy(xml, row->field(0, ""), xmlsize);

												if(nspots > 0 && markin && markout)
												{
													HBAL *h;
													if(h = (HBAL *) EMALLOC(2 * nspots * sizeof (HBAL)))
													{
														// sort spots descending
														unsigned int s, ss;
														unsigned long t;
														bool b;
														for(s = 0; s < nspots; s++)
														{
															h[s * 2].offset = pspot[s0 + s].start;
															h[s * 2].closing = false;
															h[(s * 2) + 1].offset = pspot[s0 + s].start + pspot[s0 + s].len;
															h[(s * 2) + 1].closing = true;
														}
														for(s = 0; s < (nspots * 2) - 1; s++)
														{
															for(ss = s + 1; ss < (nspots * 2); ss++)
															{
																if(h[s].offset < h[ss].offset)
																{
																	t = h[s].offset;
																	h[s].offset = h[ss].offset;
																	h[ss].offset = t;
																	b = h[s].closing;
																	h[s].closing = h[ss].closing;
																	h[ss].closing = b;
																}
															}
														}
														// insert markups
														for(s = 0; s < nspots * 2; s++)
														{
															if(h[s].closing && markout_l > 0)
															{
																memmove(xml + h[s].offset + markout_l, xml + h[s].offset, xmlsize - h[s].offset);
																memcpy(xml + h[s].offset, markout, markout_l);
																xmlsize += markout_l;
															}
															else
															{
																if(!h[s].closing && markin_l > 0)
																{
																	memmove(xml + h[s].offset + markin_l, xml + h[s].offset, xmlsize - h[s].offset);
																	memcpy(xml + h[s].offset, markin, markin_l);
																	xmlsize += markin_l;
																}
															}
														}
														EFREE(h);
													}
												}
												add_assoc_string(zanswer, (char *) "xml", xml, true);

												EFREE(xml);
											}

										}
									}
									else
									{
									}
//									conn->close();
								}
							}
							add_next_index_zval(zanswers, zanswer);
						}
						add_assoc_zval(return_value, (char *) "results", zanswers);

						if(pspot)
							EFREE(pspot);
						EFREE(panswer);

						add_assoc_double(return_value, (char *) "time_phpfct", stopChrono(time_phpfct));
					}
				}
				fclose(fp_answers);
			}
			EFREE(fname);
		}
	}
}

