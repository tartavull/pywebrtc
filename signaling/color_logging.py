import logging

logger = None

def add_colors(log_obj):
    global logger
    logger = log_obj
    NO_COLOR = "\33[m"
    RED, GREEN, ORANGE, BLUE, PURPLE, LBLUE, GREY = \
        map("\33[%dm".__mod__, range(31, 38))

    logging.basicConfig(format="%(levelname)s %(name)s %(asctime)s %(message)s", level=logging.DEBUG)

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
    global logger
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


