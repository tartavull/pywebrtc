import time
import asyncio as aio
from concurrent.futures import ThreadPoolExecutor

import sys
sys.path.append('build/')
import pybind as webrtc

async def wait_future(f):
    """
    This seems to be only necessary when 
    `f.set_result happens from a C child thread.
    I don't know why.
    """
    while True:
        await aio.sleep(.1)
        if f.done():
            return f.result()

def wait_future_sync(f):
    while True:
        time.sleep(0.1)
        if f.done():
            return f.result()

def get_offer_sync():
    f = aio.Future()
    webrtc.create_offer(f)
    return wait_future_sync(f)

def get_answer_sync(offer):
    f = aio.Future()
    webrtc.create_answer(offer, f)
    return wait_future_sync(f)

def set_answer_sync(answer):
    webrtc.set_answer(answer)

def get_candidates():
    return webrtc.get_candidates()

def set_candidates(candidates):
    return webrtc.set_candidates(candidates)

def send_data(data):
    return webrtc.send_data(data)

async def get_offer():
    loop = aio.get_running_loop()
    f = aio.Future()
    webrtc.create_offer(f)
    return await wait_future(f)

async def get_answer(offer):
    loop = aio.get_running_loop()
    f = aio.Future()
    loop.run_in_executor(executor, webrtc.create_answer, offer,  f)
    #return await f

async def main():        
    #print(await get_offer())
    print('waiting for offer')

if __name__ == '__main__':
    executor = ThreadPoolExecutor(max_workers=1)  # Prepare your executor somewhere.
    loop = aio.get_event_loop()
    try:
        aio.ensure_future(main())
        #loop.run_forever()
    finally:
        loop.run_until_complete(loop.shutdown_asyncgens())
        loop.close()
