import json
import sys

async def send_message(ws, msg_type, msg):
    await ws.send_str(json.dumps({
        'msg_type': msg_type,
        'msg': msg
    }))

async def handle_message(ws, data):
    data = json.loads(data)
    print(data)
    msg_type, msg = data['msg_type'], data['msg']
    if msg_type == '/signaling/offer':
        await send_message(ws, '/signaling/answer', 'answer')
    elif msg_type == '/signaling/answer':
        await send_message(ws, '/signaling/candidate', 'candidate')
    elif msg_type == '/signaling/candidate':
        await send_message(ws, '/command', 'close')
        sys.exit(0)
    elif msg_type == '/command':
        sys.exit(0)

async def attempt_connection(ws):
    await send_message(ws, '/signaling/offer', 'offer')
