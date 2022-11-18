# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "main.cpp"
							"WIFI/WIFI.c"
							"FireBase/FireBase.cpp"		
	           INCLUDE_DIRS "."
                    		"WIFI"	
                    		"FireBase"
             EMBED_TXTFILES "server_root_cert.pem")
