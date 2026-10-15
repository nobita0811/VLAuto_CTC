#include "stdafx.h"
#include "../GameDef.h"
#include "GameBase.h"
#include "JxCore.h"
#include "S3Client.h"
#include "VLClasses.h"

int CompareItemPos(const void* a, const void* b);

/*void FileCopy(char * src, char * dest)
{
	FILE * infile = fopen(src, "rb");
	FILE * outfile = fopen(dest, "wb");

	const int size = 16384;
	char buffer[size];

	while (!feof(infile))
	{
		int n = fread(buffer, 1, size, infile);
		fwrite(buffer, 1, n, outfile);
	}

	fflush(outfile);

	fclose(infile);
	fclose(outfile);
}*/

void CVLUtil::TestFunction(int nParam)
{
	//char szMsg[256];
	//int upExp = theMain.GetExpUpLevel(1, 72);
	//sprintf(szMsg, "GetExpUpLevel=%d", upExp);
	//SystemMessage(szMsg, 1);

	/*FileCopy("D:\\TEMP\\TaskVal.csv", "D:\\TEMP\\TaskVal0.csv");
	FILE *pLogFile = fopen("D:\\TEMP\\TaskVal.csv", "w");

	for (int i = 1020; i < 1050; i++)
	{
		int nTaskValue = g_pCurPlayer->GetTaskVal(i);
		fprintf(pLogFile, "%d\t%d\n", i, nTaskValue);
	}

	for (int i = 2570; i < 2580; i++)
	{
		int nTaskValue = g_pCurPlayer->GetTaskVal(i);
		fprintf(pLogFile, "%d\t%d\n", i, nTaskValue);
	}

	fclose(pLogFile);

	SystemMessage("TaskVal OK!", 1);*/

	/*KUiShop* pShop = (KUiShop*)GetPtrAddress(SHOP_BASE_ADD);
	if (pShop && pShop->IsVisible())
	{
		FILE* pLogFile = fopen("D:\\TEMP\\ShopList.txt", "w");
		KItemObject* pItemList = pShop->m_pObjectList;
		for (int i = 0; i < pShop->m_nObjCount; i++)
		{
			int idx = g_pBuySell->GetItemIndex(g_pCurPlayer->m_nCurShop, pItemList[i].uId);
			KItem* pItem = g_pBuySell->GetItem(idx);
			fprintf(pLogFile, "%d,%d,%d,%s\n", i, idx, pItemList[i].uId, pItem->m_CommonAttrib.szItemName);
		}
		fclose(pLogFile);
	}*/
}
