
#ifndef __LYX_HTTP_H__
#define __LYX_HTTP_H__

#include "curl/curl.h"

#include <sstream>
#include <string>
#include <map>
#include <json.hpp>

namespace lyx {
    
    inline size_t onWriteData(void * buffer, size_t size, size_t nmemb, void * userp)
    {
        std::string * str = dynamic_cast<std::string *>((std::string *)userp);
        std::cout << *str;
        str->append((char *)buffer, size * nmemb);
        return nmemb;
    }
    
    class HttpClient
    {
    private:
        bool debug = false;
        int connect_timeout = 10000;
        int socket_timeout = 10000;
        

        void appendHeaders(std::map<std::string, std::string> const & headers, curl_slist ** slist) const
        {
            std::ostringstream ostr;
            std::map<std::string, std::string>::const_iterator it;
            for(it=headers.begin(); it!=headers.end(); it++)
            {
                ostr << it->first << ":" << it->second;
                *slist = curl_slist_append(*slist, ostr.str().c_str());
                ostr.str("");
            }
        }
        
    public:
        HttpClient() = default;
        HttpClient(const HttpClient &) = delete;
        HttpClient & operator=(const HttpClient &) = delete;
        
        void setConnectTimeout(int connect_timeout)
        {
            this->connect_timeout = connect_timeout;
        }
        void setSocketTimeout(int socket_timeout)
        {
            this->socket_timeout = socket_timeout;
        }
        void setDebug(bool debug)
        {
            this->debug = debug;
        }


        int post(
                 std::string url,
                 std::map<std::string, std::string> const * params,
                 const std::string & body,
                 std::string * response) const
        {
            struct curl_slist * slist = NULL;

            CURL *curl;
            CURLcode res = CURLcode::CURLE_UNSUPPORTED_PROTOCOL;
            curl_global_init(CURL_GLOBAL_ALL);
            /* get a curl handle */
            curl = curl_easy_init();

            if(curl){
                std::map<std::string, std::string> temp_headers;
                temp_headers["Content-Type"] = "json:application/json";

                this->appendHeaders(temp_headers, &slist);



                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
                curl_easy_setopt(curl, CURLOPT_POST, true);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onWriteData);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) response);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, true);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, this->connect_timeout);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, this->socket_timeout);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
                curl_easy_setopt(curl, CURLOPT_VERBOSE, this->debug);

                res = curl_easy_perform(curl);

                curl_easy_cleanup(curl);
                curl_slist_free_all(slist);
            }

            curl_global_cleanup();
            
            return res;
        }


    };
    
}

#endif
