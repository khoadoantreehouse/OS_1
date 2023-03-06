all: main

main: enc_sever enc_client dec_sever dec_client keygen

enc_server: enc_sever.c
	gcc --std=c99 -o enc_sever enc_server.c
		
enc_client: enc_client.c
	gcc --std=c99 -o enc_client enc_client.c

dec_server: dec_sever.c
	gcc --std=c99 -o dec_sever dec_server.c

dec_client: dec_client.c
	gcc --std=c99 -o dec_client dec_client.c

keygen: keygen.c
	gcc --std=c99 -o keygen keygen.c

clean:
	rm -rf enc_sever
	rm -rf enc_client
	rm -rf dec_sever
	rm -rf dec_client
	rm -rf keygen