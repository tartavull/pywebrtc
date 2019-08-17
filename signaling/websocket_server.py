import asyncio as aio
import base64
from functools import partial
import json
import logging
import signal
import sys

import aiohttp
import aiohttp.web
from aiohttp import web

from signaling import send_message, handle_message

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

async def websocket_handler(request):
    ws = aiohttp.web.WebSocketResponse()
    await ws.prepare(request)
    async for msg in ws:
        aio.create_task(handle_message(ws, msg.data))
    sys.exit(0)
    return ws

def make_app():
    app = web.Application()
    app.router.add_get('/ws', websocket_handler)
    return app

if __name__ == '__main__':
    loop = aio.get_event_loop()
    add_signal_handler(loop)
    try:
        aiohttp.web.run_app(make_app(), port=8080)
        loop.run_forever()
    finally:
        logging.info('Cleaning up')
        loop.stop()
