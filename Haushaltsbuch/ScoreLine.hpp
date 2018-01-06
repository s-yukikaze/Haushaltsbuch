#pragma once

#include "TH155AddrDef.h"

enum SCORELINE_FILTER_MASK
{
	SCORELINE_FILTER__TIMESTAMP_BEGIN = 1UL,
	SCORELINE_FILTER__TIMESTAMP_END = SCORELINE_FILTER__TIMESTAMP_BEGIN << 1,
	SCORELINE_FILTER__P1NAME = SCORELINE_FILTER__TIMESTAMP_END << 1,
	SCORELINE_FILTER__P1NAMELIKE = SCORELINE_FILTER__P1NAME << 1,
	SCORELINE_FILTER__P1ID = SCORELINE_FILTER__P1NAMELIKE << 1,
	SCORELINE_FILTER__P1SID = SCORELINE_FILTER__P1ID << 1,
	SCORELINE_FILTER__P1WIN = SCORELINE_FILTER__P1SID << 1,
	SCORELINE_FILTER__P2NAME = SCORELINE_FILTER__P1WIN << 1,
	SCORELINE_FILTER__P2NAMELIKE = SCORELINE_FILTER__P2NAME << 1,
	SCORELINE_FILTER__P2ID = SCORELINE_FILTER__P2NAMELIKE << 1,
	SCORELINE_FILTER__P2SID = SCORELINE_FILTER__P2ID << 1,
	SCORELINE_FILTER__P2WIN = SCORELINE_FILTER__P2SID << 1,
	SCORELINE_FILTER__MAX = SCORELINE_FILTER__P2WIN,
	SCORELINE_FILTER__SQLMASK = (SCORELINE_FILTER__MAX << 1) - 1,
	SCORELINE_FILTER__LIMIT = SCORELINE_FILTER__MAX << 1
};

struct SCORELINE_FILTER_DESC
{
	int mask;
	time_t t_begin;
	time_t t_end;
	char p1name[TH155PNAME_MAX];
	int p1id;
	int p1sid;
	int p1win;
	char p2name[TH155PNAME_MAX];
	int p2id;
	int p2sid;
	int p2win;
	int limit;

	struct SCORELINE_FILTER_DESC & operator <<=(const struct SCORELINE_FILTER_DESC &src)
	{
		this->mask |= src.mask;

		if (src.mask & SCORELINE_FILTER__TIMESTAMP_BEGIN) {
			this->t_begin = src.t_begin;
		}
		if (src.mask & SCORELINE_FILTER__TIMESTAMP_END) {
			this->t_end = src.t_end;
		}
		if (src.mask & (SCORELINE_FILTER__P1NAME | SCORELINE_FILTER__P1NAMELIKE)) {
			memcpy(this->p1name, src.p1name, sizeof(this->p1name));
		}
		if (src.mask & SCORELINE_FILTER__P1ID) {
			this->p1id = src.p1id;
		}
		if (src.mask & SCORELINE_FILTER__P1SID) {
			this->p1sid = src.p1sid;
		}
		if (src.mask & SCORELINE_FILTER__P1WIN) {
			this->p1win = src.p1win;
		}
		if (src.mask & (SCORELINE_FILTER__P2NAME | SCORELINE_FILTER__P2NAMELIKE)) {
			memcpy(this->p2name, src.p2name, sizeof(this->p2name));
		}
		if (src.mask & SCORELINE_FILTER__P2ID) {
			this->p2id = src.p2id;
		}
		if (src.mask & SCORELINE_FILTER__P2SID) {
			this->p2sid = src.p2sid;
		}
		if (src.mask & SCORELINE_FILTER__P2WIN) {
			this->p2win = src.p2win;
		}
		if (src.mask & SCORELINE_FILTER__LIMIT) {
			this->limit = src.limit;
		}
		return *this;
	}
};

struct SCORELINE_ITEM
{
	time_t timestamp;
	char p1name[TH155PNAME_MAX];
	int p1id;
	int p1sid;
	int p1win;
	char p2name[TH155PNAME_MAX];
	int p2id;
	int p2sid;
	int p2win;
};

bool ScoreLine_Open(bool create);
void ScoreLine_Close();

void ScoreLine_Enter();
void ScoreLine_Leave(bool failed);

bool ScoreLine_QueryTrackRecord(SCORELINE_FILTER_DESC &filterdesc);
bool ScoreLine_QueryProfileRank(SCORELINE_FILTER_DESC &filterdesc, void(*callback)(SCORELINE_ITEM *, void *), void *user);
bool ScoreLine_QueryTrackRecordLog(SCORELINE_FILTER_DESC &filterdesc, void(*callback)(SCORELINE_ITEM *, void *), void *user);
bool ScoreLine_QueryTrackRecordTop(SCORELINE_FILTER_DESC &filterDesc, SCORELINE_ITEM &ret);

DWORD ScoreLine_Read(int p1, int p2, int idx);

bool ScoreLine_Append(SCORELINE_ITEM *item);
bool ScoreLine_Remove(time_t timestamp);

void ScoreLine_SetPath(LPCTSTR path);
LPCTSTR ScoreLine_GetPath();

