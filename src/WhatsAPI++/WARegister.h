/*
 * WARegister.h
 */

#ifndef WAREGISTER_H_
#define WAREGISTER_H_

#include <string>

struct WAToken
{
	static std::string GenerateToken(const std::string &number);
};

class WARegister
{
	static std::string GenerateIdentity(const std::string &phone);
	
	static CMStringA RequestCodeUrl(const std::string &phone);
};

#endif /* WAREGISTER_H_ */
