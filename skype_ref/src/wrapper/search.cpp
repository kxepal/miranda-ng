#include "search.h"

CSearch::CSearch(unsigned int oid, SERootObject* root, CSkypeProto *proto) : ContactSearch(oid, root)
{ 
	this->proto = proto;
}

void CSearch::OnChange(int prop)
{
	if (prop == P_CONTACT_SEARCH_STATUS)
	{
		CSearch::STATUS status;
		this->GetPropContactSearchStatus(status);
		if (status == FINISHED || status == FAILED)
		{
			this->isSeachFinished = true;
			/*if (this->proto)
				(proto->*SearchCompletedCallback)(this->hSearch);*/
		}
	}
}

void CSearch::OnNewResult(const ContactRef& contact, const uint& rankValue)
{
	/*if (this->proto)
		(proto->*ContactFindedCallback)(this->hSearch, contact->ref());*/
}

void CSearch::BlockWhileSearch()
{
	this->isSeachFinished = false;
	this->isSeachFailed = false;
	while (!this->isSeachFinished && !this->isSeachFailed) 
		Sleep(1); 
}

//void CSearch::SetProtoInfo(CSkypeProto* proto, HANDLE hSearch)
//{
//	this->proto = proto;
//	this->hSearch = hSearch;
//}

//void CSearch::SetOnSearchCompleatedCallback(OnSearchCompleted callback)
//{
//	this->SearchCompletedCallback = callback;
//}
//
//void CContactSearch::SetOnContactFindedCallback(OnContactFinded callback)
//{
//	this->ContactFindedCallback = callback;
//}