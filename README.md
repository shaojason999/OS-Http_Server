# hw2-simple-my-http-server
Complete with the http and the multithread  
Comlpete with the dir  
Without create a thread to handle sub_file and sub_dir  
Without create a sub_dir in client  
  
multithread ref: https://github.com/Guppster/MultiThreaded-Server  

Command:  
$made  
server:  
$./server –r root -p port -n thread_number  
$./server -r root -p 12345 -n 3  
client:  
$./client -t file_or_dir -h host_name -p port $./client -t /testdir/secfolder -h 127.0.0.1 -p 12345  
