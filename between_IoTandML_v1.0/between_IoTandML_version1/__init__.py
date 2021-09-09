import logging
import json 
import requests 
from datetime import datetime

import azure.functions as func

def main(req: func.HttpRequest) -> func.HttpResponse:
    logging.info('Python HTTP trigger function processed a request...\n') 

    # obtain and decode the JSON data from Azure IoT Central 
    input_stream = req.get_json()
    # logging.info(input_stream)

    # extract telemetry 
    telemetry = input_stream.get("telemetry")
    if telemetry:
        msgid = telemetry.get("MsgID") 
        people_num = telemetry.get("NumOfPeople")
        max_capability = telemetry.get("MaxCapability") 
    else:
        msgid = None 
        people_num = None 
        max_capability = None 

    # process time info
    timeinfo = input_stream.get("enqueuedTime") 
    if timeinfo:
        time_str = timeinfo.split('T')[1]
        time_list = time_str.split(':')
        time_in_hour = int(time_list[0])
        time_in_min = int(time_list[1])
    else: 
        time_in_hour = None 
        time_in_min = None 
    # obtain week day info
    week_day = datetime.now().weekday() + 1

    logging.info(f'# of people: {people_num}, MsgID: {msgid}, capability: {max_capability}, @{timeinfo}\n') 

    #------------------------AZML API------------------------
    # encaplusate ml input json data 
    ml_model_input = {
        "data": [[week_day, time_in_hour, time_in_min, people_num]] 
    }

    data = json.dumps(ml_model_input) 

    # url of deployed Azure ML model 
    uri = 'http://be61aad3-0d12-44d3-9cd0-04ebed5dfeeb.uksouth.azurecontainer.io/score'
    headers = {"Content-Type": "application/json"}

    ml_request = requests.post(uri, data = data, headers = headers)

    predicted_NoP = int(ml_request.json()[0]) # receive the predicted result 
    logging.info(f'ml predicted result = {predicted_NoP}') 
    #------------------------AZML API------------------------

    #------------------------IoT central API------------------------
    if predicted_NoP >= max_capability: 
        url = 'https://room-capability-monitor.azureiotcentral.com/api/devices/798fe1082451e644bf3003f3b1ab7b5faa9c8ea14771eb3f78d7369aa2ccd90207a72da2fc83e9fcd77fdfd97ea679102a7da5cbd4de06c25bbfda4c4494445a/commands/OverflowWarning?api-version=1.0' 
    else: 
        url = 'https://room-capability-monitor.azureiotcentral.com/api/devices/798fe1082451e644bf3003f3b1ab7b5faa9c8ea14771eb3f78d7369aa2ccd90207a72da2fc83e9fcd77fdfd97ea679102a7da5cbd4de06c25bbfda4c4494445a/commands/CancelWarning?api-version=1.0' 
    myheaders = {
        "Authorization": "SharedAccessSignature sr=0286bfe4-e5ec-483d-96cb-0a4449dde8f4&sig=rK2erTEEpx8JB201IziZkFIXWhVNOI2c7O0t86CHAXo%3D&skn=myToken&se=1659184896941",
        "Content-type": "application/json" 
    }
    playload = {} 
    azsphere_requests = requests.post(url, headers = myheaders, json = playload) 
    logging.info(azsphere_requests.content) 
    #------------------------IoT central API------------------------
    # return func.HttpResponse('Hello, test sucessfully!') 
    return func.HttpResponse(f'The predicted result = {predicted_NoP}, {azsphere_requests.content}.') 

