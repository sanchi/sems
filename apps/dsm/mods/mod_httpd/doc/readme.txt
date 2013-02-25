
create call:

curl -d msgType=CreateCall -d msg=\{\"scCallId\":\"scCallId1\",\"timeout\":90,\"from\":\"sip:100@192.168.5.110:5080\",\"to\":\"sip:UNKNOWN@192.168.5.110\"\} http://127.0.0.1:7090/session/create

curl -d msgType=CreateCall -d msg=\{\"scCallId\":\"scCallId1\",\"timeout\":90,\"from\":\"sip:100@192.168.5.110:5080\",\"to\":\"sip:UNKNOWN@192.168.5.110\"\} http://127.0.0.1:7090/session/create
