#pragma once

#include "common.h"

class CTransfer : public Transfer
{
public:
	typedef DRef<CTransfer, Transfer> Ref;
	typedef DRefs<CTransfer, Transfer> Refs;

	CTransfer(unsigned int oid, SERootObject* root, CSkypeProto *proto);

private:
	CSkypeProto *proto;

	void OnChange(int prop);
};