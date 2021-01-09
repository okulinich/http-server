#!/bin/bash





#### TEST 1
### bad uri-s
## start
#
testBadUri() {
    echo -e "\e[4mTEST 1\e[24m\n(bad uri-s)\n"
    # creaating very long uri for a test
    tooLongUri="a"
    for i in {0..2000}
    do
        tooLongUri="${tooLongUri}a"
    done

    # array that stores examples of bad uri-s, that should cause errors
    declare -a uri=("/absent_file" "/forbidden_file" "*" "/~/../../dev/null" $tooLongUri)
    # array that stores corresponding status codes to invalid uri-s
    declare -a statusCode=("404" "403" "501" "403" "414")

    # making requests with all the uri-s and checking if status codes
    # in responces are correct
    for i in {0..4}
    do
        response=$(printf "GET ${uri[i]} HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 3819 | tr -d '\0')
        result=$(echo $response | grep -c ${statusCode[i]})
        echo -n "> Expected ${statusCode[i]} status code for uri: "
        if [[ $i == 4 ]] 
        then
            echo -e "\e[94mof size 2000\e[39m"
        else
            echo -e "\e[94m${uri[i]}\e[39m"
        fi
        if [[ $result == 0 ]]
        then
            echo -e "\e[31m!FAIL\e[39m\n"
        else
            echo -e "\e[32m!SUCCESS\e[39m\n"
        fi
    done
    rm "$1/resources/forbidden_file"
    sleep 1
}
#
## end
### bad uri-s
#### TEST 1





#### TEST 2
### http ver-s
## start
#
testHttpVer() {
    echo -e "\n\e[4mTEST 2\e[24m\n(http ver-s)\n"
    # responses with valid & invalid http versions
    declare -a httpVerTest=("GET /index.html HTTP/2.0\r\nHost: localhost\r\n\r\n" "GET /index.html HTTP/1.0\r\n\r\n" "GET /index.html HTTP/1.1\r\n\r\n" "GET /index.html HTTP/3.5\r\n\r\n")
    # corresponding status codes
    declare -a statusCode=("501" "200" "400" "400")
    # sending each response and checking wether the requests has corresponding 
    # status code or not
    for i in {0..3}
    do
        response=$(printf "${httpVerTest[i]}" | nc localhost 3819 | tr -d '\0')
        result=$(echo $response | grep -c ${statusCode[i]})
        # format output of request and expected status code
        echo -en "Sent: \e[94m"
        echo -n ${httpVerTest[i]}
        echo -e "\e[39m"
        echo -e "Expected \e[94m${statusCode[i]}\e[39m" 

        if [[ $result == 1 ]]
        then 
            echo -e "\e[32m!SUCCESS\e[39m\n"
        else
            echo -e "\e[31m!FAIL\e[39m\n"
        fi
    done
    sleep 1
}
#
## end
### http ver-s
#### TEST 2





#### TEST 3
### partial req-s
## start
#
testPartialReq() {
    echo -e "\n\e[4mTEST 3\e[24m\n(partial req-s)\n"
    # Array with partial requests (they all are invalid)
    declare -a partialRequests=("GET\r\n\r\n" "GET /\r\n\r\n" "GET /index.html HTTP\r\n\r\n" "GET /index.html HTTP/1\r\n\r\n" "GET /index.html HTTP/1.\r\n\r\n")
    # sending each request and waiting for response 400 Bad Request
    for i in {0..4}
    do
        response=$(printf "${partialRequests[i]}" | nc localhost 3819 | tr -d '\0')
        result=$(echo $response | grep -c 400)

        echo -en "Partial request $(($i+1)): \e[94m"
        echo -n ${partialRequests[i]}
        echo -e "\e[39m"

        if [[ $result == 1 ]]
        then
            echo -e "\e[32m!SUCCESS\e[39m\n"
        else
            echo -e "\e[31m!FAIL\e[39m\n"
        fi
    done
    sleep 1
}
#
## end
### partial req-s
#### TEST 3





#### TEST 4
### keep-alive
## start
#
testKeepAlive1() {
    echo -e "\n\e[4mTEST 4\e[24m\n(keep-alive: general)\n"
    # sending response with Keep-Alive header and timeout parameter(10 sec), 
    # calculating how long the connection has been open
    echo -en "\e[5mAnalysis...\e[0m"
    startTime=$(date +%s)
    printf "GET /index.html HTTP/1.0\r\nConnection: keep-alive\r\nKeep-Alive: timeout=10, max=5\r\n\r\n" | nc localhost 3819 > temp_file
    endTime=$(date +%s)
    elapsedTime=$(($endTime-$startTime))
    # comparing expected time and really elapsed time
    echo -e "\e[K\rAnalysis finished"
    echo -e "Expected time: \e[94m10 sec\e[39m" 
    echo -e "Elapsed time: \e[94m$elapsedTime\e[39m"
    if [[ $elapsedTime == 10 ]]
    then
        echo -e "\e[32mKeep-Alive works properly\e[39m"
    else
        echo -e "\e[31mKeep-Alive doesn't work properly\e[39m"
    fi
    rm temp_file
    sleep 1
}
#
## end
### keep-alive
#### TEST 4





#### TEST 5
### massive keep-alive
## start
#
testKeepAlive2() {
    echo -e "\n\n\e[4mTEST 5\e[24m\n(keep-alive: massive)\n"
    # sending 100 requests with Keep-Alive header and timeout parameter(60 sec), hihing every connection to bachground
    for i in {1..100}
    do
        echo -ne "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\nKeep-Alive: timeout=60, max=2\r\n\r\n" | nc localhost 3819 >> respFile &
    done
    # calculating the number of connections in background
    bgConnections=$(ps | grep -c nc)
    echo -e "> Connections in background now: \e[94m$bgConnections\e[39m"
    echo -en "\e[5mAnalysis...\e[0m"
    sleep 5
    echo -e "\e[K\rAnalysis finished"
    # calculating the number of correct responses
    echo -en "> Correct responses: \e[94m"
    respAmount=$(cat respFile | grep -c "200 OK")
    echo $respAmount
    echo -en "\e[39m"
    if [[ $bgConnections -le $respAmount ]]
    then
        echo -e "\e[32m!SUCCESS\e[39m"
    else
        echo -e "\e[31m!FAIL\e[39m\nNo responses for $(($bgConnections-$respAmount)) connections"
    fi
    # keeping everything clean
    rm respFile
    killall nc
    sleep 1
}
#
## end
### massive keep-alive
#### TEST 5





#### TEST 6 / TEST 7 
### (keep-alive: timeout) / (paralel connections)
## start
#
testKeepAlive3() {
    gcc -o test_6_7 -Wno-pointer-to-int-cast -pthread test_6_7.c
    ./test_6_7
    rm test_6_7
}
#
## end
### (keep-alive: timeout) / (paralel connections)
#### TEST 6 / TEST 7





#### TEST 8
### video downloading
## start
#
testHugeFileWithMd5() {
    echo -e "\n\n\e[4mTEST 8\e[24m\n(video downloading)\n"
    originalFileSum=$(md5sum $1/resources/vidrap.mp4 | cut -d ' ' -f1)
    numberOfTimes=10
    downloaded=0
    failed=0
    missedParts=0
    for i in {1..10}
    do
        echo -ne "Downloading video for \e[94m$i\e[39m time\033[0K\r";

        curl -s http://localhost:3819/vidrap.mp4 > videx.mp4
        videoFileSum=$(md5sum videx.mp4 | cut -d ' ' -f1)
        
        if [ -z $videoFileSum ]
        then 
            failed=$((failed+1))
        elif [[ $videoFileSum == $originalFileSum ]]
        then 
            downloaded=$((downloaded+1))
        else
            missedParts=$((missedParts+1))
        fi
    done
    # results of the test
    echo -e "\e[32mDownloaded\e[39m \e[94m$downloaded\e[39m full videos from \e[94m$numberOfTimes\e[39m"
    echo -e "\e[31mFailed\e[39m to download \e[94m$failed\e[39m videos"
    echo -e "\e[35mMissed\e[39m parts from \e[94m$missedParts\e[39m videos"
    rm videx.mp4
}
#
## end
### video downloading
#### TEST 7





#### TEST 9
### chunked encoding
## start
#
testChunked() {
    echo -e "\n\n\e[4mTEST 9\e[24m\n(chunked encoding)\n"
    curl -s --dump-header headFile -o videx.mp4 http://localhost:3819/vidrap.mp4
    chunkedEnabled=$(cat headFile | grep "Transfer-Encoding: chunked")
    noContentLength=$(cat headFile | grep "Content-Length")

    if [[ -z "${noContentLength}" && ! -z "${chunkedEnabled}" ]]
    then
        echo -e "\e[32m!SUCCESS:\e[39m Chunked transfer encoding was used for a large file"
    else
        echo -e "\e[31m!FAIL:\e[39m Chunked transfer encoding wasn't used for a large file"
    fi

    curl -s --dump-header headFile -o homePage http://localhost:3819/
    chunkedEnabled=$(cat headFile | grep "Transfer-Encoding: chunked")
    noContentLength=$(cat headFile | grep "Content-Length")
    if [[ ! -z "${noContentLength}" && -z "${chunkedEnabled}" ]]
    then
        echo -e "\e[32m!SUCCESS:\e[39m Chunked transfer encoding wasn't used for a small file"
    else
        echo -e "\e[31m!FAIL:\e[39m Chunked transfer encoding was used for a small file"
    fi

    rm videx.mp4
    rm homePage
    rm headFile
}
#
## end
### chunked encoding
#### TEST 9
