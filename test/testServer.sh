#!/bin/bash

                #   First things first: checking server for valid error handling.   #
                #       We will check very basic things: absent file, file          #
                #       with no perissions, * uri, uri with wildcards, too          #
                #       long uri and just random invalid text in request            #
                #   Then checking different HTTP versions, invalid partial requests #
                #       keep-alive attribute, parallel connections, big             #
                #       files downloading and chunked transfer encoding             #

if [ ! -n "$1" ]
then
    echo "Specify path to server folder please!"
    exit 1
fi
# creating file with no suitable permissions
# to test error case
touch "$1/resources/forbidden_file"
chmod a=w "$1/resources/forbidden_file"

echo -e "\e[5m\e[34m\t\tTEST STARTED\e[25m\e[39m\n\n"
sleep 1


source testScripts.sh

testBadUri
testHttpVer
testPartialReq
testKeepAlive1
testKeepAlive2
testKeepAlive3
testHugeFileWithMd5
testChunked


echo -e "\e[5m\e[34m\t\tTEST COMPLETED\e[25m\e[39m\n\n"
sleep 1
