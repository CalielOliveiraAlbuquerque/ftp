server: 
	g++ server.cpp -o app -lavcodec -lavutil -lX11 -lswscale

client:
	g++ client.cpp -o app -lavcodec -lX11 

docker:
	docker build -t rtp-server .

	xhost +local:root

	docker run -it \
	  --net=host \
	  -env="DISPLAY=$$DISPLAY" \
	  --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
	  rtp-server

