#include "pch.hpp"
#include "ScoreLine.hpp"
#include "TH155AddrDef.h"

#include "sqlite3.h"
#include <utility>

#include "MinimalMemory.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

#define TRACKRECORD_TABLE "trackrecord155"

static sqlite3 *s_db;
static bool s_viewEntered;

static DWORD s_scoreLine[TH155CHAR_MAX][TH155CHAR_MAX][2];
static DWORD s_scoreLineNew[TH155CHAR_MAX][TH155CHAR_MAX][2];
static Minimal::ProcessHeapString  s_srPath;
static Minimal::ProcessHeapStringA s_srPathU;

void ScoreLine_SetPath(LPCTSTR path)
{
	s_srPath = path;
	Minimal::ToUTF8(s_srPathU, path);
}

LPCTSTR ScoreLine_GetPath()
{
	return s_srPath;
}

static void ScoreLine_UserFunc_AnsiToUTF8(sqlite3_context *context, int argc, sqlite3_value **argv) {
	if (argc >= 1) {
		const char *source = reinterpret_cast<const char *>(sqlite3_value_text(argv[0]));
		Minimal::ProcessHeapStringA utf8ized;
		Minimal::ToUTF8(utf8ized, source);
		sqlite3_result_text(context, utf8ized.GetRaw(), utf8ized.GetSize(), SQLITE_TRANSIENT);
	}
}

static int ScoreLine_QueryCallback(void *, int argc, char **argv, char **colName)
{
	if (argc != 4) return 1;

	int p1id = ::StrToIntA(argv[0]);
	int p2id = ::StrToIntA(argv[1]);
	int p1win = ::StrToIntA(argv[2]);
	int p2win = ::StrToIntA(argv[3]);

	if (p1id < 0 || p1id >= TH155AddrGetCharCount()) return 1;
	if (p2id < 0 || p2id >= TH155AddrGetCharCount()) return 1;
	if (p1win < 0) p1win = 0;
	if (p2win < 0) p2win = 0;

	s_scoreLineNew[p1id][p2id][0] = p1win;
	s_scoreLineNew[p1id][p2id][1] = p2win;
	return 0;
}

static int ScoreLine_QueryCallback2(void *user, int argc, char **argv, char **colName)
{
	if (argc != 3) return 1;

	SCORELINE_ITEM item;
	::lstrcpyA(item.p2name, argv[0]);
	item.p1win = ::StrToIntA(argv[1]);
	item.p2win = ::StrToIntA(argv[2]);

	std::pair<void(*)(SCORELINE_ITEM *, void *), void*> *cbinfo;
	*(void**)&cbinfo = user;

	cbinfo->first(&item, cbinfo->second);

	return 0;
}

static int ScoreLine_QueryCallback3(void *user, int argc, char **argv, char **colName)
{
	if (argc != 9) return 1;

	SCORELINE_ITEM item;
	::StrToInt64ExA(argv[0], STIF_DEFAULT, &item.timestamp);
	::lstrcpyA(item.p1name, argv[1]);
	item.p1id  = ::StrToIntA(argv[2]);
	item.p1sid = ::StrToIntA(argv[3]);
	item.p1win = ::StrToIntA(argv[4]);
	::lstrcpyA(item.p2name, argv[5]);
	item.p2id = ::StrToIntA(argv[6]);
	item.p2sid = ::StrToIntA(argv[7]);
	item.p2win = ::StrToIntA(argv[8]);

	std::pair<void(*)(SCORELINE_ITEM *, void *), void*> *cbinfo;
	*(void**)&cbinfo = user;

	cbinfo->first(&item, cbinfo->second);

	return 0;
}

static int ScoreLine_QueryCallback4(void *user, int argc, char **argv, char **colName)
{
	if (argc != 9) return 1;

	struct {
		bool processed;
		SCORELINE_ITEM *pret;
	} *pretpack;
	*(void **)&pretpack = user;
	
	SCORELINE_ITEM *pitem = pretpack->pret;
	::StrToInt64ExA(argv[0], STIF_DEFAULT, &pitem->timestamp);
	::lstrcpyA(pitem->p1name, argv[1]);
	pitem->p1id  = ::StrToIntA(argv[2]);
	pitem->p1sid = ::StrToIntA(argv[3]);
	pitem->p1win = ::StrToIntA(argv[4]);
	::lstrcpyA(pitem->p2name, argv[5]);
	pitem->p2id  = ::StrToIntA(argv[6]);
	pitem->p2sid = ::StrToIntA(argv[7]);
	pitem->p2win = ::StrToIntA(argv[8]);

	pretpack->processed = true;
	return 0;
}

static void ScoreLine_ConstructFilter(Minimal::ProcessHeapStringA &ret, SCORELINE_FILTER_DESC &filterDesc)
{
	if ((filterDesc.mask & SCORELINE_FILTER__SQLMASK) == 0) return;
	char buff[128];
	ret += " WHERE ";

	int count = 0;
	for (int i = 1; i <= SCORELINE_FILTER__MAX; i <<= 1) {
		if (count > 0 && (filterDesc.mask & i)) ret += " AND ";
		switch(filterDesc.mask & i) {
		case SCORELINE_FILTER__P1NAME:
			sqlite3_snprintf(_countof(buff), buff, "p1name = '%q'", filterDesc.p1name);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P2NAME:
			sqlite3_snprintf(_countof(buff), buff, "p2name = '%q'", filterDesc.p2name);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P1NAMELIKE:
			sqlite3_snprintf(_countof(buff), buff, "ansi2utf8(p1name) LIKE '%q'", static_cast<LPCSTR>(MinimalA2UTF8(filterDesc.p1name)));
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P2NAMELIKE:
			sqlite3_snprintf(_countof(buff), buff, "ansi2utf8(p2name) LIKE '%q'", static_cast<LPCSTR>(MinimalA2UTF8(filterDesc.p2name)));
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P1ID:
			sqlite3_snprintf(_countof(buff), buff, "p1id = %ld", filterDesc.p1id);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P1SID:
			sqlite3_snprintf(_countof(buff), buff, "p1sid = %ld", filterDesc.p1sid);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P2ID:
			sqlite3_snprintf(_countof(buff), buff, "p2id = %ld", filterDesc.p2id);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P2SID:
			sqlite3_snprintf(_countof(buff), buff, "p2sid = %ld", filterDesc.p2sid);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P1WIN:
			sqlite3_snprintf(_countof(buff), buff, "p1win = %ld", filterDesc.p1win);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__P2WIN:
			sqlite3_snprintf(_countof(buff), buff, "p2win = %ld", filterDesc.p2win);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__TIMESTAMP_BEGIN:
			sqlite3_snprintf(_countof(buff), buff, "timestamp >= %lld", filterDesc.t_begin);
			ret += buff;
			++count;
			break;
		case SCORELINE_FILTER__TIMESTAMP_END:
			sqlite3_snprintf(_countof(buff), buff, "timestamp <= %lld", filterDesc.t_end);
			ret += buff;
			++count;
			break;
		}
	}
}

bool ScoreLine_QueryTrackRecord(SCORELINE_FILTER_DESC &filterDesc)
{

	Minimal::ProcessHeapStringA filterStr;
	char *errmsg;
	int rc;

	ScoreLine_ConstructFilter(filterStr, filterDesc);

	ZeroMemory(s_scoreLineNew, sizeof s_scoreLineNew);
	char *query = sqlite3_mprintf(
		(filterDesc.mask & SCORELINE_FILTER__LIMIT ?
			"SELECT T.p1id, T.p2id, sum(T.p1win/2), sum(T.p2win/2) "
			"FROM ("
				"SELECT p1id, p2id, p1win, p2win "
				"FROM " TRACKRECORD_TABLE " %s ORDER BY timestamp DESC LIMIT %d"
			") AS T "
			"GROUP BY T.p1id, T.p2id;"
			:
			"SELECT T.p1id, T.p2id, sum(T.p1win/2), sum(T.p2win/2) "
			"FROM " TRACKRECORD_TABLE " AS T %s GROUP BY T.p1id, T.p2id;"
		),
		filterStr.GetRaw(), filterDesc.limit);
	rc = sqlite3_exec(s_db, 
		query, ScoreLine_QueryCallback, NULL, &errmsg);
	sqlite3_free(query);
	if (rc) return false;

	memcpy(s_scoreLine, s_scoreLineNew, sizeof(s_scoreLine));
	return true;
}

DWORD ScoreLine_Read(int p1, int p2, int idx)
{
	return s_scoreLine[p1][p2][idx];
}

bool ScoreLine_Append(SCORELINE_ITEM *item)
{
	sqlite3_stmt *stmt;
	int rc;

	if (!s_db) return false;

	rc = sqlite3_prepare(s_db, 
		"insert into " TRACKRECORD_TABLE " (timestamp, p1name, p1id, p1sid, p1win, p2name, p2id, p2sid, p2win) "
		 "values (?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &stmt, NULL);
	if (rc) return false;

	sqlite3_bind_int64(stmt, 1, item->timestamp);
	sqlite3_bind_text (stmt, 2, item->p1name, -1, NULL);
	sqlite3_bind_int  (stmt, 3, item->p1id);
	sqlite3_bind_int  (stmt, 4, item->p1sid);
	sqlite3_bind_int  (stmt, 5, item->p1win);
	sqlite3_bind_text (stmt, 6, item->p2name, -1, NULL);
	sqlite3_bind_int  (stmt, 7, item->p2id);
	sqlite3_bind_int  (stmt, 8, item->p2sid);
	sqlite3_bind_int  (stmt, 9, item->p2win);
	bool ret = (sqlite3_step(stmt) == SQLITE_DONE);

	sqlite3_finalize(stmt);

	return ret;
}

bool ScoreLine_Remove(time_t timestamp)
{
	if (!s_db) return false;
	char *query = sqlite3_mprintf(
		"DELETE FROM " TRACKRECORD_TABLE " WHERE timestamp = %lld",
		timestamp);
	int rc = sqlite3_exec(s_db, query, NULL, NULL, NULL);
	sqlite3_free(query);
	if (rc) return false;

	return true;
}

bool ScoreLine_QueryProfileRank(SCORELINE_FILTER_DESC &filterDesc, void(*callback)(SCORELINE_ITEM *, void *), void *user)
{
	if (!s_db) return false;

	Minimal::ProcessHeapStringT<char> filterStr;
	ScoreLine_ConstructFilter(filterStr, filterDesc);

	std::pair<void(*)(SCORELINE_ITEM *, void *), void*> cbinfo
		= std::make_pair(callback, user);
	char *query;
	if (filterDesc.mask & SCORELINE_FILTER__LIMIT) {
		query = sqlite3_mprintf(
			"SELECT T.p2name, sum(T.p1win/2), sum(T.p2win/2) "
			"FROM ("
				"SELECT p2name, p1id, p2id, p1win, p2win "
				"FROM " TRACKRECORD_TABLE " ORDER BY timestamp DESC LIMIT %d"
			") AS T "
			"%s GROUP BY T.p2name",
			filterDesc.limit, filterStr.GetRaw());
	} else {
		query = sqlite3_mprintf(
			"SELECT T.p2name, sum(T.p1win/2), sum(T.p2win/2) "
			"FROM " TRACKRECORD_TABLE " AS T %s GROUP BY p2name",
			filterStr.GetRaw());
	}
	int rc = sqlite3_exec(s_db, query, ScoreLine_QueryCallback2, (void*)&cbinfo, NULL);
	sqlite3_free(query);
	if (rc) return false;

	return true;
}

bool ScoreLine_QueryTrackRecordLog(SCORELINE_FILTER_DESC &filterDesc, void(*callback)(SCORELINE_ITEM *, void *), void *user)
{
	if (!s_db) return false;

	Minimal::ProcessHeapStringT<char> filterStr;
	ScoreLine_ConstructFilter(filterStr, filterDesc);

	std::pair<void(*)(SCORELINE_ITEM *, void *), void*> cbinfo
		= std::make_pair(callback, user);

	char *query = sqlite3_mprintf(
		(filterDesc.mask & SCORELINE_FILTER__LIMIT ?
			"SELECT timestamp, p1name, p1id, p1sid, p1win, p2name, p2id, p2sid, p2win FROM " TRACKRECORD_TABLE " %s ORDER BY timestamp DESC LIMIT %d"
			:
			"SELECT timestamp, p1name, p1id, p1sid, p1win, p2name, p2id, p2sid, p2win FROM " TRACKRECORD_TABLE " %s ORDER BY timestamp DESC"
		),
		filterStr.GetRaw(), filterDesc.limit);
	int rc = sqlite3_exec(s_db, query, ScoreLine_QueryCallback3, (void*)&cbinfo, NULL);
	sqlite3_free(query);
	if (rc) return false;

	return true;

}

bool ScoreLine_QueryTrackRecordTop(SCORELINE_FILTER_DESC &filterDesc, SCORELINE_ITEM &ret)
{
	if (!s_db) return false;

	Minimal::ProcessHeapStringA filterStr;
	ScoreLine_ConstructFilter(filterStr, filterDesc);

	char *query = sqlite3_mprintf(
		"SELECT timestamp, p1name, p1id, p1sid, p1win, p2name, p2id, p2sid, p2win FROM " TRACKRECORD_TABLE " %s ORDER BY timestamp DESC LIMIT 1",
		filterStr.GetRaw());

	struct {
		bool processed;
		SCORELINE_ITEM *pret;
	} retpack = { false, &ret };

	int rc = ::sqlite3_exec(s_db, query, ScoreLine_QueryCallback4, (void *)&retpack, NULL);
	sqlite3_free(query);
	if (rc) return false;

	return retpack.processed;
}

bool ScoreLine_Open(bool create)
{
	sqlite3 *db;
	if (PathFileExists(s_srPath)) {
		// ÉIÅ[ÉvÉìèàóù
		int rc = sqlite3_open(s_srPathU, &db);
		if (rc) {
			sqlite3_close(db);
			return false;
		}
		char *errmsg;
		rc = sqlite3_exec(db,
			"PRAGMA temp_store = MEMORY;"
			"PRAGMA cache_size = 1048576;"
			, NULL, NULL, &errmsg);
		if (rc) {
			sqlite3_close(db);
			return false;
		}

		rc = sqlite3_create_function(db,
			"ansi2utf8", 1, SQLITE_UTF8,
			static_cast<void *>(db),
			ScoreLine_UserFunc_AnsiToUTF8, NULL, NULL);
		if (rc) {
			sqlite3_close(db);
			return false;
		}
	} else {
		// çÏê¨èâä˙âªèàóù
		int rc = sqlite3_open(s_srPathU, &db);
		if (rc) {
			sqlite3_close(db);
			return false;
		}
		char *errmsg;
		rc = sqlite3_exec(db, 
			"CREATE TABLE " TRACKRECORD_TABLE "(\n"
				"timestamp	INTEGER NOT NULL,\n"
				"p1name     TEXT,\n"
				"p1id       INTEGER NOT NULL,\n"
				"p1sid      INTEGER NOT NULL,\n"
				"p1win      INTEGER NOT NULL,\n"
				"p2name     TEXT,\n"
				"p2id       INTEGER NOT NULL,\n"
				"p2sid      INTEGER NOT NULL,\n"
				"p2win      INTEGER NOT NULL,\n"
				"PRIMARY KEY (timestamp)\n"
			")", NULL, NULL, &errmsg);
		if (rc) {
			sqlite3_close(db);
			::DeleteFile(s_srPath);
			return false;
		}
		rc = sqlite3_exec(db, 
			"CREATE INDEX " TRACKRECORD_TABLE "_index\n"
			"on " TRACKRECORD_TABLE " (timestamp)", NULL, NULL, &errmsg);
		if (rc) {
			sqlite3_close(db);
			::DeleteFile(s_srPath);
			return false;
		}
		rc = sqlite3_exec(db,
			"PRAGMA temp_store = MEMORY;"
			"PRAGMA cache_size = 1048576;"
			, NULL, NULL, &errmsg);
		if (rc) {
			sqlite3_close(db);
			::DeleteFile(s_srPath);
			return false;
		}

		rc = sqlite3_create_function(db,
			"ansi2utf8", 1, SQLITE_UTF8,
			static_cast<void *>(db),
			ScoreLine_UserFunc_AnsiToUTF8, NULL, NULL);
		if (rc) {
			sqlite3_close(db);
			::DeleteFile(s_srPath);
			return false;
		}
	}

	ScoreLine_Close();
	s_db = db;

	return true;
}

void ScoreLine_Enter()
{
	if (!s_db) return;
	sqlite3_exec(s_db, "BEGIN", NULL, NULL, NULL);
}

void ScoreLine_Leave(bool failed)
{
	if (!s_db) return;
	sqlite3_exec(s_db, failed ? "ROLLBACK" : "COMMIT", NULL, NULL, NULL);
}

void ScoreLine_Close()
{
	if (s_db) {
		sqlite3_close(s_db);
		s_db = NULL;
	}
}
