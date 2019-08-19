import json
import asyncio as aio
import sys
import time
import logging

sys.path.append('../build')
import pybind as webrtc


NO_COLOR = "\33[m"
RED, GREEN, ORANGE, BLUE, PURPLE, LBLUE, GREY = \
    map("\33[%dm".__mod__, range(31, 38))

logging.basicConfig(format="%(levelname)s %(name)s %(asctime)s %(message)s", level=logging.DEBUG)
logger = logging.getLogger('pywebrtc')

def add_color(logger_method, color):
  def wrapper(message, *args, **kwargs):
    return logger_method(
      # the coloring is applied here.
      color+message+NO_COLOR,
      *args, **kwargs
    )
  return wrapper

for level, color in zip(("debug", "info", "warn", "error", "critical"), (GREEN, BLUE, ORANGE, RED, PURPLE)):
  setattr(logger, level, add_color(getattr(logger, level), color))

def log(level, msg):
    if level == 'debug':
        logger.debug(msg)
    elif level == 'info':
        logger.info(msg)
    elif level == 'warn':
        logger.warn(msg)
    elif level == 'error':
        logger.error(msg)
    elif level == 'critical':
        logger.critial(msg)

webrtc.set_logging_callback(log)


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
    logger.info('sent:' + json.dumps({
        'msg_type': msg_type,
        'msg': msg}))

async def send_data(data):
    webrtc.send_data(data)


async def handle_message(ws, data):
    global candidates_sent
    logger.info('recieved:'+ data)
    data = json.loads(data)
    msg_type, msg = data['msg_type'], data['msg']
    if msg_type == '/signaling/offer':
        await send_message(ws, '/signaling/answer', create_answer(msg))
    elif msg_type == '/signaling/answer':
        webrtc.set_answer(msg)
        await send_message(ws, '/signaling/candidate', webrtc.get_candidates())
        candidates_sent = True
    elif msg_type == '/signaling/candidate':
        logging.info('recieved candidate and candidate_sent='+str(candidates_sent))
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
