import asyncio as aio
import base64
import json
import logging
from functools import partial
import signal

import aiohttp
import aiohttp.web
from aiohttp import web

from signaling import send_data, send_message, handle_message, attempt_connection

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s,%(msecs)d %(levelname)s: %(message)s',
    datefmt='%H:%M:%S',
)

def add_signal_handler(loop):
    signals = (signal.SIGHUP, signal.SIGTERM, signal.SIGINT)
    for s in signals:
        loop.add_signal_handler(
            s, lambda s=s: aio.create_task(shutdown(s, loop)))

async def shutdown(signal, loop):
    logging.info(f'Received exit signal {signal.name}...')
    loop.stop()
    logging.info('Shutdown complete.')

async def connect_websocket():
    async with aiohttp.ClientSession() as session:
        async with session.ws_connect('http://localhost:8080/ws') as ws:
            await attempt_connection(ws)
            async for msg in ws:
                aio.create_task(handle_message(ws, msg.data))


async def heartbeat():
    while True:
        await aio.sleep(1)
        print('heartbeat')
        payload = {
            'storage_available': 2**30,
            'battery_voltage': 7.5,
            'current_used': 7.5,
            'lattitude': 43.6532,
            'longitude': 79.3832,
            'speed': 3.4,
            'RSSI': -50,
        }
        loop.create_task(send_data(json.dumps(payload)))

async def main():
    aio.create_task(connect_websocket())
    loop.create_task(heartbeat())

if __name__ == '__main__':
    loop = aio.get_event_loop()
    add_signal_handler(loop)
    try:
        loop.create_task(main())
        loop.run_forever()
    finally:
        logging.info('Cleaning up')
        loop.stop()
