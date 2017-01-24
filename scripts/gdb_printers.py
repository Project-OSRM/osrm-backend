import gdb.printing

# https://sourceware.org/gdb/onlinedocs/gdb/Pretty-Printing.html
# https://sourceware.org/gdb/onlinedocs/gdb/Writing-a-Pretty_002dPrinter.html

COORDINATE_PRECISION = 1e6

class CoordinatePrinter:
    """Print a CoordinatePrinter object."""
    def __init__(self, val):
        self.val = val

    def to_string(self):
        lon, lat = int(self.val['lon']['__value']), int(self.val['lat']['__value'])
        return '{{{}, {}}}'.format(float(lon) / COORDINATE_PRECISION, float(lat) / COORDINATE_PRECISION)

class TurnInstructionPrinter:
    """Print a TurnInstruction object."""

    modifiers = {0:'UTurn', 1:'SharpRight', 2:'Right', 3:'SlightRight',
                 4:'Straight', 5:'SlightLeft', 6:'Left', 7:'SharpLeft'}
    types = {0:'Invalid', 1:'NewName', 2:'Continue', 3:'Turn', 4:'Merge', 5:'OnRamp',
             6:'OffRamp', 7:'Fork', 8:'EndOfRoad', 9:'Notification', 10:'EnterRoundabout',
             11:'EnterAndExitRoundabout', 12:'EnterRotary', 13:'EnterAndExitRotary',
             14:'EnterRoundaboutIntersection', 15:'EnterAndExitRoundaboutIntersection',
             16:'UseLane', 17:'NoTurn', 18:'Suppressed', 19:'EnterRoundaboutAtExit',
             20:'ExitRoundabout', 21:'EnterRotaryAtExit', 22:'ExitRotary',
             23:'EnterRoundaboutIntersectionAtExit', 24:'ExitRoundaboutIntersection',
             25:'StayOnRoundabout', 26:'Sliproad'}

    def __init__(self, val):
        self.val = val

    def to_string(self):
        t, m = int(self.val['type']), int(self.val['direction_modifier'])
        m = '%s (%d)' % (self.modifiers[m], m) if m in self.modifiers else str(m)
        t = '%s (%d)' % (self.types[t], t) if t in self.types else str(t)
        return '{{type = {}, direction_modifier = {}}}'.format(t, m)

class TurnLaneDataPrinter:
    """Print a TurnLaneData object."""

    mask = {0:'Empty', 1:'None', 2:'Straight', 4:'SharpLeft', 8:'Left', 16:'SlightLeft',
            32:'SlightRight', 64:'Right', 128:'SharpRight', 256:'UTurn', 512:'MergeToLeft',
            1024:'MergeToRight'}

    def __init__(self, val):
        self.val = val

    def to_string(self):
        tg = int(self.val['tag'])
        fr, to = int(self.val['from']), int(self.val['to'])
        return '{{tag = {}, from = {}, to = {}}}'.format(self.mask[tg] if tg in self.mask else tg, fr, to)

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('OSRM')
    pp.add_printer('TurnInstruction', '::TurnInstruction$', TurnInstructionPrinter)
    pp.add_printer('Coordinate', '::Coordinate$', CoordinatePrinter)
    pp.add_printer('TurnLaneData', '::TurnLaneData$', TurnLaneDataPrinter)
    return pp

#gdb.pretty_printers = [filter(lambda x: x.name != 'OSRM', gdb.pretty_printers)]
gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
