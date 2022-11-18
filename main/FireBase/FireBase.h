/*
 * FireBase.h
 *
 *  Created on: 21 thg 4, 2022
 *      Author: A315-56
 */

#ifndef _FIREBASE_H_
#define _FIREBASE_H_


#include <string.h>
#include <iostream>
#include <string>
#include "esp_http_client.h"


#ifdef __cplusplus
extern "C" {
#endif

using namespace std;


typedef struct FireBase_Auth{
	string Username;
	string Password;
	string Api_Key;
	string Auth_Secrets = " ";
} FB_Auth;

class FireBase{
	public:
		FireBase(void);
		void Init(string URL);
		void Init(string URL, const char *Centificate);
		void Denit(void);
		void Config(FireBase_Auth *Auth);
		void StartAcction(void);
		void StopAcction (void);

		void SetInt   (string Path, int DataInt);
		void SetDouble(string Path, double DataFloat);
		void SetBool  (string Path, bool DataBool);
		void SetString(string Path, string DataString);
		void SetJson  (string Path, string Json);

		int    GetInt   (string Path);
		double GetDouble(string Path);
		bool   GetBool  (string Path);
		string GetString(string Path);
		string GetJson  (string Path);
		string GetJsonValue(string Input_Json, string Key);
		int  JsonKey_to_Int(string Input_Json, string Key);
		bool JsonKey_to_Bool(string Input_Json, string Key);
		double JsonKey_to_Double(string Input_Json, string Key);

		void Delete(string Path);

		void esp_httpclient_send(char *URL, char *Data, esp_http_client_method_t METHOD);
		char *PRJ_URL = NULL;

	private:
		void GetIDToken(string API_KEY);
		int find_lastof_chr(char *Source, char chr);
		void Get_Path(char *Source);
		void Get_Name(char *Source);
		void free_mem(void);

		char *URL = NULL, *DATA = NULL;
		char Path_t[50] = {'\0'};
		char Name_t[50] = {'\0'};

		esp_http_client_config_t config_post = {};
};

#ifdef __cplusplus
}
#endif

#endif /* _FIREBASE_H_ */
