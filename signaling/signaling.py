import json
import asyncio as aio
import sys
import time

sys.path.append('../build')
import pybind as webrtc

candidates_sent = False

def wait_future_sync(f):
    while True:
        time.sleep(0.1)
        if f.done():
            return f.result()

def create_offer():
    f = aio.Future()
    webrtc.create_offer(f)
    return wait_future_sync(f)

def create_answer(offer):
    f = aio.Future()
    webrtc.create_answer(offer, f)
    return wait_future_sync(f)

async def send_message(ws, msg_type, msg):
    await ws.send_str(json.dumps({
        'msg_type': msg_type,
        'msg': msg
    }))
    print('sent:' + json.dumps({
        'msg_type': msg_type,
        'msg': msg}))

async def send_data(data):
    webrtc.send_data(data)


async def handle_message(ws, data):
    global candidates_sent
    print('recieved:'+ data)
    data = json.loads(data)
    msg_type, msg = data['msg_type'], data['msg']
    if msg_type == '/signaling/offer':
        await send_message(ws, '/signaling/answer', create_answer(msg))
    elif msg_type == '/signaling/answer':
        webrtc.set_answer(msg)
        await send_message(ws, '/signaling/candidate', webrtc.get_candidates())
        candidates_sent = True
    elif msg_type == '/signaling/candidate':
        webrtc.set_candidates(msg)
        if not candidates_sent:
            await send_message(ws, '/signaling/candidate', webrtc.get_candidates())
            candidates_sent = True
        #await send_message(ws, '/command', 'close')
        #sys.exit(0)
    elif msg_type == '/command':
        sys.exit(0)

async def attempt_connection(ws):
    await send_message(ws, '/signaling/offer', create_offer())
