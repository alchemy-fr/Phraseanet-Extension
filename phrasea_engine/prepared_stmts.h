/*
SQL_SB_0 : sqltrec = "record"
SQL_SB_1 : sqltrec = "(record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND ((status ^ mask_xor) & mask_and)=0)"

SQL_MODE_DOC (documents only) : parent_record_id=0
SQL_MODE_GRP (regroups only)  : parent_record_id=record.record_id
*/
char *prepsql[PHRASEA_NBR_OPS][2][2][2] =
{
		// 0 : undefined
		{ { {NULL, NULL}, {NULL, NULL} }, { {NULL, NULL}, {NULL, NULL} } },

		// 1 : PHRASEA_OP_OR
		{ { {NULL, NULL}, {NULL, NULL} }, { {NULL, NULL}, {NULL, NULL} } },

		// 2 : PHRASEA_OP_AND
		{ { {NULL, NULL}, {NULL, NULL} }, { {NULL, NULL}, {NULL, NULL} } },

		// 3 : PHRASEA_KW_ALL
		{
				// no status-bits
				{
						// doc only
						{
								// no sort
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								" FROM record WHERE record.parent_record_id=0",

								// sorted
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								", prop.value AS skey, prop.type AS styp"
								" FROM record"
								" INNER JOIN prop ON(record.parent_record_id=0 AND prop.record_id=record.record_id AND prop.name=?)"
						},
						// reg only
						{
								// no sort
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								" FROM record WHERE record.parent_record_id=record.record_id",

								// sorted
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								", prop.value AS skey, prop.type AS styp"
								" FROM record"
								" INNER JOIN prop ON(record.parent_record_id=record.record_id AND prop.record_id=record.record_id AND prop.name=?)"
						}
				},
				// with status-bits
				{
						// doc only
						{
								// no sort
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								" FROM (record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND ((status ^ mask_xor) & mask_and)=0) WHERE record.parent_record_id=0",
								// sorted
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								", prop.value AS skey, prop.type AS styp"
								" FROM (record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND ((status ^ mask_xor) & mask_and)=0)"
								" INNER JOIN prop ON(record.parent_record_id=0 AND prop.record_id=record.record_id AND prop.name=?)"
						},
						// reg only
						{
								// no sort
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								" FROM (record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND ((status ^ mask_xor) & mask_and)=0) WHERE record.parent_record_id=record.record_id",

								// sorted
								"SELECT record.record_id, record.parent_record_id, record.coll_id, status"
								", prop.value AS skey, prop.type AS styp"
								" FROM (record INNER JOIN _tmpmask ON _tmpmask.coll_id=record.coll_id AND ((status ^ mask_xor) & mask_and)=0)"
								" INNER JOIN prop ON(record.parent_record_id=record.record_id AND prop.record_id=record.record_id AND prop.name=?)"
						}
				}
		},



		// 3 : PHRASEA_KW_ALL
		{
				// no status-bits
				{
						// doc only
						{
								// no sort
								"",
								// sorted
								""
						},
						// reg only
						{
								// no sort
								"",
								// sorted
								""
						}
				},
				// with status-bits
				{
						// doc only
						{
								// no sort
								"",
								// sorted
								""
						},
						// reg only
						{
								// no sort
								"",
								// sorted
								""
						}
				}
		},
};

