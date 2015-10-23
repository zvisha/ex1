c_server:

  cd c_server

  make

  ./server 8080



server:

  cd server

  node ddos_server.js 8080




client:

  cd client

  node ddos_client.js 10 1000 8080
