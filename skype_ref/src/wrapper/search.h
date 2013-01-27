#pragma once

#include "common.h"

class CSearch : public ContactSearch
{
public:

	typedef DRef<CSearch, ContactSearch> Ref;
	typedef DRefs<CSearch, ContactSearch> Refs;
	
	bool isSeachFinished;
	bool isSeachFailed;

	CSearch(unsigned int oid, SERootObject* root, CSkypeProto *proto);

	void OnChange(int prop);
	void OnNewResult(const ContactRef& contact, const uint& rankValue);

	/*void SetProtoInfo(CSkypeProto* proto, HANDLE hSearch);
	void SetOnSearchCompleatedCallback(OnSearchCompleted callback);
	void SetOnContactFindedCallback(OnContactFinded callback);*/

	void BlockWhileSearch();

private:
	CSkypeProto *proto;
	HANDLE hSearch;
	/*CSkypeProto* proto;
	OnSearchCompleted SearchCompletedCallback;
	OnContactFinded ContactFindedCallback;*/
};