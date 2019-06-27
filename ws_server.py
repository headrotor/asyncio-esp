import asyncio
import websockets
import logging
import time

# for debugging websocket states
logger = logging.getLogger('websockets')
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())

# Websocket server for esp32 or similar hardware. 
# Bidirection asynchrounous data transfer over wifi, using msgpack
# for complex data serialization/deserialization


server_URL = 'scrut.local'
server_port = 8765
refresh_time = 5

# User facing functions here 



async def producer():
    # producer function, called repeatedly, returns data to send to client 
    # asyncio.sleep to pause

    ddict = {}
    ddict['time'] = time.strftime("%H:%M:%S", time.localtime()) 
    return(msgpack.packb(ddict))

    #bus_data = BusLogic(b12)
    #print(str(bus_data.get_route_dict()))
    #return(msgpack.packb(bus_data.get_route_dict()))

async def consumer(message, ws):
    # consumer function, called with message data sent from client
    print("message: {}".format(message))
    if message == 'butA':
        mpl.client.pause()
        print('butA')
    elif message == 'butA_long':
        print('butA long')
        mpl.client.stop()
    elif message == 'butB':
        mpl.client.next()
    elif message == 'butC':
        mpl.client.previous()
    # refresh display
    ddict ={}
    ddict['time'] = time.strftime("%H:%M:%S", time.localtime()) 
    logger.info("sending c")
    await ws.send(msgpack.packb(ddict))
    logger.info("done sending c")


################################################

async def producer_handler(websocket, path):
    while True:
        await asyncio.sleep(refresh_time)
        message = await producer()
        await websocket.send(message)

async def consumer_handler(ws, path):
    async for message in ws:
        await consumer(message, ws)
        logger.info("ws message recieved")

 
async def ws_handler(ws, path):
    consumer_task = asyncio.ensure_future(
        consumer_handler(ws, path))
    producer_task = asyncio.ensure_future(
        producer_handler(ws, path))
    done, pending = await asyncio.wait(
        [consumer_task, producer_task],
        return_when=asyncio.FIRST_COMPLETED,
    )
    for task in done:
        # this is where we catch exceptions...
        try:
            task.result()
        except Exception as e:
            logger.info("Caught excpetion!")
            raise e
    for task in pending:
        task.cancel()


ws = websockets.serve(ws_handler, server_URL, server_port)
loop = asyncio.get_event_loop()
loop.run_until_complete(ws)

# if you need to put something else in the event loop
#task = loop.create_task(repeat_sendsock(ws))

loop.run_forever()
