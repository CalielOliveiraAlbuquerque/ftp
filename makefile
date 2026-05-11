server: 
	g++ server.cpp -o app -lavcodec -lavutil -lX11 -lswscale

client:
	g++ client.cpp -o app -lavcodec -lX11 

