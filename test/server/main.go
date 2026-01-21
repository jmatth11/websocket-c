package main

import (
	"log"
	"net/http"
	"os"
	"strings"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
}

func wsHandler(w http.ResponseWriter, r *http.Request) {
	// Upgrade the HTTP connection to a WebSocket connection
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
		return
	}
	defer conn.Close()

	// Connection is established over WSS if the server is running HTTPS
	// Handle the connection (read/write messages, etc.)
	for {
		// Read message from the client
		messageType, p, err := conn.ReadMessage()
		if err != nil {
			log.Println("read:", err)
			break
		}
		log.Printf("recv: type%v -- %s\n", messageType, p)

		// Write message back to the client (echo example)
		err = conn.WriteMessage(messageType, p)
		if err != nil {
			log.Println("write:", err)
			break
		}
		log.Printf("write: %s\n", p)
	}
}

func main() {
	http.HandleFunc("/ws", wsHandler)
	use_tls := false
	if len(os.Args) > 1 {
		val := strings.TrimSpace(os.Args[1])
		if val == "tls" {
			use_tls = true
		}
		if val == "help" {
			log.Printf("Usage: \n- tls : launch server with tls\n- help : print this message\n")
			os.Exit(0)
		}
	}

	// Use http.ListenAndServeTLS to enable WSS
	// You need valid certificate and key files
	var err error
	if use_tls {
		err = http.ListenAndServeTLS(":443", "../server.crt", "../server.key", nil)
	} else {
		err = http.ListenAndServe(":80", nil)
	}
	if err != nil {
		log.Fatal("ListenAndServe error: ", err)
	}
}
