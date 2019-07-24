#include "maprotation.h"

#include <vector>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <base/system.h>
#include <base/math.h>

class CRandomMapRotation: public IMapRotation 
{
public: 
	class CMap
	{
	public:
		const char* m_pName;
		float m_Probability;

		CMap(const char* name, float weight);
	};
	std::vector<CMap> m_MapList;
	const char* m_pMapFile;
	time_t m_MapFileMTime;

	CRandomMapRotation();

	virtual const char* NextMap(const char* rotation, const char* current);
	void UpdateList(const char* rotation);
	void ClearList();
};

time_t GetMTime(const char* file)
{
	struct stat st;
	if (stat(file, &st) < 0)
	{
		dbg_msg("rotation", "Cannot stat %s: %s", file, strerror(errno));
		return -1;
	}
	return st.st_mtime;
}

CRandomMapRotation::CMap::CMap(const char* name, float probability)
: m_pName(name), m_Probability(probability) {}

CRandomMapRotation::CRandomMapRotation()
: m_pMapFile(NULL) {};

void CRandomMapRotation::UpdateList(const char* rotation)
{
	if (m_pMapFile && str_comp(rotation, m_pMapFile) == 0)
	{
		time_t mtime = GetMTime(m_pMapFile);
		if (mtime < 0 || mtime == m_MapFileMTime)
			return;
		m_MapFileMTime = mtime;
	}
	else
	{
		time_t mtime = GetMTime(rotation);
		if (mtime < 0) return;
		m_pMapFile = rotation;
		m_MapFileMTime = mtime;
	}

	ClearList();

	std::ifstream f(m_pMapFile);
	while (1)
	{
		char buf[128];
		float weight;
		f >> buf >> weight;
		if (!f) break;
		char *name = new char[strlen(buf) + 1];
		if (!name) break;
		strcpy(name, buf);
		m_MapList.push_back(CMap(name, weight));
	}

	// normalize probabilities
	float norm = 0;
	for (std::vector<CMap>::const_iterator m = m_MapList.begin(); m != m_MapList.end(); ++m)
		norm += m->m_Probability;
	for (std::vector<CMap>::iterator m = m_MapList.begin(); m != m_MapList.end(); ++m)
		m->m_Probability /= norm;

	for (std::vector<CMap>::const_iterator m = m_MapList.begin(); m != m_MapList.end(); ++m)
		dbg_msg("rotation", "registering map %s with probability %g", m->m_pName, m->m_Probability);
}

void CRandomMapRotation::ClearList()
{
	for (std::vector<CMap>::const_iterator m = m_MapList.begin(); m != m_MapList.end(); ++m)
		delete[] m->m_pName;
	m_MapList.clear();
}

const char* CRandomMapRotation::NextMap(const char* rotation, const char* current)
{
	UpdateList(rotation);
	if (m_MapList.empty()) return current;
	float w = frandom();
	float x = 0;
	for (std::vector<CMap>::const_iterator m = m_MapList.begin(); m != m_MapList.end(); ++m)
	{
		x += m->m_Probability;
		if (x >= w) return m->m_pName;
	}
	// should never happen
	return current;
}

IMapRotation *RandomMapRotation()
{
	return new CRandomMapRotation;
}
