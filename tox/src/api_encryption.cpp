#include "common.h"

/* ENCRYPTION FUNCTIONS */

uint32_t tox_encrypted_size(const Tox *tox)
{
	return CreateFunction<uint32_t(*)(const Tox*)>(__FUNCTION__)(tox);
}

Tox *tox_encrypted_new(const struct Tox_Options *options, const uint8_t *data, size_t length, uint8_t *passphrase, size_t pplength, TOX_ERR_NEW *error)
{
	return CreateFunction<Tox*(*)(const struct Tox_Options*, const uint8_t*, size_t, uint8_t*, size_t, TOX_ERR_NEW*)>(__FUNCTION__)(options, data, length, passphrase, pplength, error);
}

int tox_encrypted_save(const Tox *tox, uint8_t *data, uint8_t *passphrase, uint32_t pplength)
{
	return CreateFunction<int(*)(const Tox*, const uint8_t*, uint8_t*, uint32_t)>(__FUNCTION__)(tox, data, passphrase, pplength);
}

int tox_is_data_encrypted(const uint8_t *data)
{
	return CreateFunction<int(*)(const uint8_t*)>(__FUNCTION__)(data);
}