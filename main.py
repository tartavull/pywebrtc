import pybind as webrtc
import asyncio as aio
from concurrent.futures import ThreadPoolExecutor

async def get_offer():
    loop = aio.get_running_loop()
    f = aio.Future()
    loop.run_in_executor(executor, webrtc.get_offer, f)
    return await f

async def get_answer():
    loop = aio.get_running_loop()
    f = aio.Future()
    loop.run_in_executor(executor, webrtc.get_answer, f)
    return await f

async def main():        
    f = aio.Future()
    print(await get_offer())

if __name__ == '__main__':
    executor = ThreadPoolExecutor(max_workers=1)  # Prepare your executor somewhere.
    loop = aio.get_event_loop()
    try:
        aio.ensure_future(main())
        loop.run_forever()
    finally:
        loop.run_until_complete(loop.shutdown_asyncgens())
        loop.close()
