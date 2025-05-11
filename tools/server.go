package main

import (
	"fmt"
	"net/http"
)

const serverAddr = "localhost:8090"

func handlerHello(w http.ResponseWriter, req *http.Request) {
	fmt.Printf("Received request for %q\n", req.URL.Path)

	fmt.Fprintf(w, "hello\n")
}

func handlerHeaders(w http.ResponseWriter, req *http.Request) {
	fmt.Printf("Received request for %q\n", req.URL.Path)

	requestHeaders := "Request Headers:\n"

	for name, headers := range req.Header {
		for _, h := range headers {
			requestHeaders += fmt.Sprintf("%v: %v\n", name, h)
		}
	}

	fmt.Fprintf(w, requestHeaders)
}

func main() {

	http.HandleFunc("/hello", handlerHello)
	http.HandleFunc("/headers", handlerHeaders)

	fmt.Printf("Server listening on %q...\n", serverAddr)
	http.ListenAndServe(serverAddr, nil)
}
