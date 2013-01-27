#include "transfer.h"

CTransfer::CTransfer(unsigned int oid, SERootObject* root, CSkypeProto *proto) : Transfer(oid, root)
{ 
	this->proto = proto;
}

//void CTransfer::SetOnTransferChangedCallback(OnTransferChanged callback, CSkypeProto* proto)
//{
//	this->proto = proto;
//	this->callback = callback;
//}

void CTransfer::OnChange(int prop)
{
	/*if (this->proto)
		(proto->*callback)(this->ref(), prop);*/
}