#pragma once

class KNode
{
public:
	KNode* m_pNext;
	KNode* m_pPrev;

public:
	KNode(void);
	virtual ~KNode(){};
	KNode* GetNext(void);
	KNode* GetPrev(void);
};

inline KNode::KNode(void)
{
	m_pNext = NULL;
	m_pPrev = NULL;
}

inline KNode* KNode::GetNext(void)
{
	return m_pNext;
}

inline KNode* KNode::GetPrev(void)
{
	return m_pPrev;
}
