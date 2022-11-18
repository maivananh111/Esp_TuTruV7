/*
 * FireBase.c
 *
 *  Created on: 21 thg 4, 2022
 *      Author: A315-56
 */
#include "stdlib.h"
#include "FireBase.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/timer.h"
#include "esp_crt_bundle.h"



//char IDToken[400] = {'\0'};
char *User = NULL;
char *Pass = NULL;
char *Secrets = NULL;
esp_http_client_handle_t client = NULL;
static const char *TAG = "FIREBASE HTTP CLIENT";
static char *DATA_RESP;
static bool Sign_State = false;
static bool Authentication = false;

esp_err_t firebase_event_handler(esp_http_client_event_handle_t evt);
esp_err_t get_auth_event_handler(esp_http_client_event_handle_t evt);


FireBase::FireBase(void){}

esp_err_t firebase_event_handler(esp_http_client_event_handle_t evt){
    switch(evt->event_id){
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
		break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
		break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
		break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
		break;
		case HTTP_EVENT_ON_DATA:
			asprintf(&DATA_RESP, "%s", (char *)evt->data);
			ESP_LOGI("FIREBASE DATA", "Responsed status: %d, Length: %d", esp_http_client_get_status_code(client),  evt->data_len);
//			ESP_LOGI(TAG, "HTTP: Response code: %d, Length: %d, Data = %s\n", esp_http_client_get_status_code(client),  evt->data_len, (char *)evt->data);
		break;
		default:
		break;
    }
    return ESP_OK;
}

esp_err_t get_auth_event_handler(esp_http_client_event_handle_t evt){
    if(evt->event_id == HTTP_EVENT_ON_DATA){
		asprintf(&DATA_RESP, "%s", (char *)evt->data);
		ESP_LOGI(TAG, "GOOGLEAPIS ACCOUNT: Response code: %d, Length: %d, Data = %s\n", esp_http_client_get_status_code(client),  evt->data_len, (char *)evt->data);
		if(esp_http_client_get_status_code(client) == 200) Sign_State = true;
    }
    return ESP_OK;
}

void FireBase::StartAcction(void){
	client = esp_http_client_init(&config_post);
}

void FireBase::StopAcction(void){
    esp_http_client_cleanup(client);
}
 /* Gá»�i hÃ m nÃ y trÆ°á»›c hÃ m Init */
void FireBase::Config(FireBase_Auth *Auth){
	char *URL;
	asprintf(&User, "%s", Auth->Username.c_str());
	asprintf(&Pass, "%s", Auth->Password.c_str());
	if(Auth->Auth_Secrets.compare(" ") != 0){
		asprintf(&Secrets, "%s", Auth->Auth_Secrets.c_str());
		Authentication = true;
	}
	asprintf(&URL, "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%s", Auth->Api_Key.c_str());
	esp_http_client_config_t post = {};
	post.url = (const char *)URL;
	post.method = HTTP_METHOD_POST;
	post.crt_bundle_attach = esp_crt_bundle_attach;
	post.event_handler = get_auth_event_handler;
//	post.cert_pem = (const char *)server_root_cert_pem_start;

    client = esp_http_client_init(&post);
    char *post_data;
    asprintf(&post_data, "{\"email\": \"%s\", \"password\": \"%s\", \"returnSecureToken\": true}", Auth->Username.c_str(), Auth->Password.c_str());
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Connection", "close");
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    free(URL);
    free(post_data);
}

void FireBase::Init(string URL){
	asprintf(&PRJ_URL,  "%s", URL.c_str());
    config_post.url = PRJ_URL;
	config_post.method = HTTP_METHOD_GET;
	config_post.transport_type = HTTP_TRANSPORT_OVER_TCP;
	config_post.crt_bundle_attach = esp_crt_bundle_attach;
	config_post.event_handler = firebase_event_handler;

    vTaskDelay(5/portTICK_PERIOD_MS);
    StartAcction();
}

void FireBase::Init(string URL, const char *Centificate){
	asprintf(&PRJ_URL,  "%s", URL.c_str());
    config_post.url = PRJ_URL;
	config_post.method = HTTP_METHOD_GET;
	config_post.transport_type = HTTP_TRANSPORT_OVER_TCP;
	config_post.cert_pem = Centificate;
	config_post.event_handler = firebase_event_handler;

    vTaskDelay(5/portTICK_PERIOD_MS);
    StartAcction();
}

void FireBase::Denit(void){
	StopAcction();
	if(User != NULL && Pass != NULL){
		free(User);
		free(Pass);
	}
}

void FireBase::esp_httpclient_send(char *URL, char *Data, esp_http_client_method_t METHOD){
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, METHOD);
    esp_http_client_set_post_field(client, Data, strlen(Data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_perform(client);
}

int FireBase::find_lastof_chr(char *Source, char chr){
    int index = -1;
    for (int i = 0; i < strlen(Source); i++)
        if (Source[i] == chr)
            index = i;
    return index;
}

void FireBase::Get_Path(char *Source){
	memset(Path_t, '\0', 50);
	int index = find_lastof_chr(Source, '/');
	for(int i=0; i<index; i++){
		Path_t[i] = Source[i];
	}
}

void FireBase::Get_Name(char *Source){
	memset(Name_t, '\0', 50);
	int index = find_lastof_chr(Source, '/');
	for(int i=index+1; i<strlen(Source); i++){
		Name_t[i - (index+1)] = Source[i];
	}
}

void FireBase::free_mem(void){
	free(DATA_RESP);
	free(URL);
	free(DATA);
}

/* POST_PUT_PACTH DATA */
void FireBase::SetInt(string Path, int DataInt){
	if(Authentication == true && Sign_State == true){
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json?auth=%s", PRJ_URL, Secrets);
			asprintf(&DATA, "{\"%s\": %d}", Path.c_str(), DataInt);
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
			asprintf(&DATA, "{\"%s\": %d}", Name_t, DataInt);
		}
	}
	else{
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json", PRJ_URL);
			asprintf(&DATA, "{\"%s\": %d}", Path.c_str(), DataInt);
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
			asprintf(&DATA, "{\"%s\": %d}", Name_t, DataInt);
		}
	}
//	Get_Path((char *)Path.c_str());
//	Get_Name((char *)Path.c_str());
//	if(Authentication == true && Sign_State == true) asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
//	else asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
//	asprintf(&DATA, "{\"%s\": %d}", Name_t, DataInt);
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase::SetDouble(string Path, double DataFloat){
	if(Authentication == true && Sign_State == true){
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json?auth=%s", PRJ_URL, Secrets);
			asprintf(&DATA, "{\"%s\": %f}", Path.c_str(), DataFloat);
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
			asprintf(&DATA, "{\"%s\": %f}", Name_t, DataFloat);
		}
	}
	else{
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json", PRJ_URL);
			asprintf(&DATA, "{\"%s\": %f}", Path.c_str(), DataFloat);
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
			asprintf(&DATA, "{\"%s\": %f}", Name_t, DataFloat);
		}
	}
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase::SetBool(string Path, bool DataBool){
	if(Authentication == true && Sign_State == true){
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json?auth=%s", PRJ_URL, Secrets);
			if(DataBool) asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), "true");
			else asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), "false");
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
			if(DataBool) asprintf(&DATA, "{\"%s\": %s}", Name_t, "true");
			else asprintf(&DATA, "{\"%s\": %s}", Name_t, "false");
		}
	}
	else{
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json", PRJ_URL);
			if(DataBool) asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), "true");
			else asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), "false");
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
			if(DataBool) asprintf(&DATA, "{\"%s\": %s}", Name_t, "true");
			else asprintf(&DATA, "{\"%s\": %s}", Name_t, "false");
		}
	}
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase::SetString(string Path, string DataString){
	if(Authentication == true && Sign_State == true){
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json?auth=%s", PRJ_URL, Secrets);
			asprintf(&DATA, "{\"%s\": \"%s\"}", Path.c_str(), DataString.c_str());
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
			asprintf(&DATA, "{\"%s\": \"%s\"}", Name_t, DataString.c_str());
		}
	}
	else{
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json", PRJ_URL);
			asprintf(&DATA, "{\"%s\": \"%s\"}", Path.c_str(), DataString.c_str());
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
			asprintf(&DATA, "{\"%s\": \"%s\"}", Name_t, DataString.c_str());
		}
	}
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase::SetJson(string Path, string Json){
	if(Authentication == true && Sign_State == true){
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json?auth=%s", PRJ_URL, Secrets);
			asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), Json.c_str());
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path_t, Secrets);
			asprintf(&DATA, "{\"%s\": %s}", Name_t, Json.c_str());
		}
	}
	else{
		if(Path.find("/") == -1){
			asprintf(&URL, "%s/.json", PRJ_URL);
			asprintf(&DATA, "{\"%s\": %s}", Path.c_str(), Json.c_str());
		}
		else{
			Get_Path((char *)Path.c_str());
			Get_Name((char *)Path.c_str());
			asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
			asprintf(&DATA, "{\"%s\": %s}", Name_t, Json.c_str());
		}
	}
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

/* DELETE DATA */
void FireBase::Delete(string Path){
	asprintf(&DATA, "%s", (char *)" ");
	asprintf(&URL, "%s/%s/.json", PRJ_URL, Path.c_str());
	esp_httpclient_send(URL, DATA,HTTP_METHOD_DELETE);
	free_mem();
}

/* GET DATA */
int FireBase::GetInt(string Path){
	asprintf(&DATA, "%s", (char *)" ");
	if(Authentication == true && Sign_State == true) asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path.c_str(), Secrets);
	else asprintf(&URL, "%s/%s/.json", PRJ_URL, Path.c_str());
	esp_httpclient_send(URL, DATA,HTTP_METHOD_GET);
	int temp = atoi(DATA_RESP);
	free_mem();
	return temp;
}

double FireBase::GetDouble(string Path){
	asprintf(&DATA, "%s", (char *)" ");
	if(Authentication == true && Sign_State == true) asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path.c_str(), Secrets);
	else asprintf(&URL, "%s/%s/.json", PRJ_URL, Path.c_str());
	esp_httpclient_send(URL, DATA,HTTP_METHOD_GET);
	double temp = atof(DATA_RESP);
	free_mem();
	return temp;
}

bool FireBase::GetBool(string Path){
	asprintf(&DATA, "%s", (char *)" ");
	if(Authentication == true && Sign_State == true) asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path.c_str(), Secrets);
	else asprintf(&URL, "%s/%s/.json", PRJ_URL, Path.c_str());
	esp_httpclient_send(URL, DATA,HTTP_METHOD_GET);
	bool temp = false;
	if(strcmp(DATA_RESP, "true") == 0) temp = true;
	else temp = false;
	free_mem();
	return temp;
}

string FireBase::GetString(string Path){
	asprintf(&DATA, "%s", (char *)" ");
	if(Authentication == true && Sign_State == true) asprintf(&URL, "%s/%s/.json?auth=%s", PRJ_URL, Path.c_str(), Secrets);
	else asprintf(&URL, "%s/%s/.json", PRJ_URL, Path.c_str());
	esp_httpclient_send(URL, DATA,HTTP_METHOD_GET);
	string Get_String = "NULL";
	Get_String.clear();
	for(int i=1; i<strlen(DATA_RESP)-1; i++)
		Get_String += DATA_RESP[i];

	free_mem();
	return Get_String;
}

string FireBase::GetJson(string Path){
	return GetString(Path);
}

string FireBase::GetJsonValue(string Input_Json, string Key){
	string JsonValue;
	int Key_index = Input_Json.find(Key);
	/* Json form: "Key1":value1,"Key2":value2; */
	if(Key_index > 0){
		int Value_index = Key_index + Key.length() + 2; // 2 is char '"' and ':' after Key name.
		if(Input_Json[Value_index] == '"') Value_index++;
		while(Input_Json[Value_index] != ',' && Input_Json[Value_index + 1] != '"'){
			JsonValue += Input_Json[Value_index++];
		}
		JsonValue += Input_Json[Value_index];
		return JsonValue;
	}
	return "lol me ko co gia tri nay";
}
int FireBase::JsonKey_to_Int(string Input_Json, string Key){
	string kq = GetJsonValue(Input_Json, Key);
	if(kq.compare("lol me ko co gia tri nay") == 0){
		return INT_MIN;
	}
	return atoi(kq.c_str());
}

bool FireBase::JsonKey_to_Bool(string Input_Json, string Key){
	string kq = GetJsonValue(Input_Json, Key);
	if(!kq.compare("lol me ko co gia tri nay") || !kq.compare("false")){
		return false;
	}
	return true;
}

double FireBase::JsonKey_to_Double(string Input_Json, string Key){
	string kq = GetJsonValue(Input_Json, Key);
	if(kq.compare("lol me ko co gia tri nay") == 0){
		return (double)INT_MIN;
	}
	return atof(kq.c_str());
}






























