#!/usr/bin/env python3
"""Execute une ROM MD avec un core libretro local et sauvegarde une capture.
Usage: run_headless.py CORE.so ROM.bin OUTPUT.png [--frames 180] [--buttons 7]
Les boutons sont des identifiants libretro, presses 5 images apres le demarrage.
"""
import argparse
import ctypes as C
from pathlib import Path
from PIL import Image


def run(core_path, rom_path, output, frames=180, buttons=(), observe=None,
        script=None, memory_observer=None, startup=None):
    core = C.CDLL(str(core_path.resolve()))
    state = {'format': 2, 'frame': None, 'buttons': set()}
    directory = C.create_string_buffer(str(output.parent.resolve()).encode())
    Env = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
    Video = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
    Audio = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
    Batch = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
    Poll = C.CFUNCTYPE(None)
    Input = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)

    @Env
    def environment(command, data):
        if command == 10:  # SET_PIXEL_FORMAT
            state['format'] = C.cast(data, C.POINTER(C.c_int))[0]
            return state['format'] in (0, 1, 2)
        if command in (9, 31):
            C.cast(data, C.POINTER(C.c_void_p))[0] = C.addressof(directory)
            return True
        if command == 3:
            C.cast(data, C.POINTER(C.c_bool))[0] = True
            return True
        if command == 39:
            C.cast(data, C.POINTER(C.c_uint))[0] = 0
            return True
        if command in (11, 16, 18, 35, 37):
            return True
        return False

    @Video
    def video(data, width, height, pitch):
        if not data or data == C.c_void_p(-1).value:
            return
        size = 4 if state['format'] == 1 else 2
        raw = b''.join(C.string_at(data + y * pitch, width * size) for y in range(height))
        state['frame'] = (width, height, raw)

    def to_image():
        if state['frame'] is None:
            raise RuntimeError('Aucune image recue')
        width, height, raw = state['frame']
        if state['format'] == 1:
            image = Image.frombytes('RGB', (width, height), raw, 'raw', 'BGRX')
        else:
            pixels = []
            for y in range(height):
                row = (C.c_uint16 * width).from_buffer_copy(raw[y * width * 2:(y + 1) * width * 2])
                for v in row:
                    if state['format'] == 2:
                        pixels.append((((v >> 11) & 31) * 255 // 31,
                                       ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31))
                    else:
                        pixels.append((((v >> 10) & 31) * 255 // 31,
                                       ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31))
            image = Image.new('RGB', (width, height))
            image.putdata(pixels)
        return image

    @Audio
    def audio(left, right):
        pass

    @Batch
    def batch(data, count):
        return count

    @Poll
    def poll():
        pass

    @Input
    def input_state(port, device, index, button):
        return int(port == 0 and button in state['buttons'])

    class Game(C.Structure):
        _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                    ('size', C.c_size_t), ('meta', C.c_char_p)]

    for name, callback in [('environment', environment), ('video_refresh', video),
                           ('audio_sample', audio), ('audio_sample_batch', batch),
                           ('input_poll', poll), ('input_state', input_state)]:
        getattr(core, 'retro_set_' + name)(callback)
    core.retro_load_game.argtypes = [C.POINTER(Game)]
    core.retro_load_game.restype = C.c_bool
    core.retro_get_memory_data.argtypes = [C.c_uint]
    core.retro_get_memory_data.restype = C.c_void_p
    core.retro_get_memory_size.argtypes = [C.c_uint]
    core.retro_get_memory_size.restype = C.c_size_t
    core.retro_init()
    raw = rom_path.read_bytes()
    buffer = C.create_string_buffer(raw)
    game = Game(str(rom_path.resolve()).encode(), C.cast(buffer, C.c_void_p), len(raw), None)
    if not core.retro_load_game(C.byref(game)):
        raise RuntimeError('Core libretro: chargement refuse')
    core.retro_set_controller_port_device(0, 1)
    for _ in range(frames):
        core.retro_run()
    for keys, duration in startup or ():
        state['buttons'] = set(keys)
        for _ in range(duration): core.retro_run()
    def capture(step):
        if observe:
            observe(to_image(), step)
        if memory_observer:
            pointer = core.retro_get_memory_data(2)  # RETRO_MEMORY_SYSTEM_RAM
            size = core.retro_get_memory_size(2)
            if not pointer or not size:
                raise RuntimeError('Core did not expose system RAM')
            memory_observer(C.string_at(pointer, size), step)
    capture(0)
    # Each requested button is a separate press, followed by a release.
    for step, button in enumerate(buttons, 1):
        state['buttons'] = {button}
        for _ in range(5): core.retro_run()
        state['buttons'] = set()
        for _ in range(5): core.retro_run()
        capture(step)
    if script:
        for step, (keys, duration) in enumerate(script, 1):
            state['buttons'] = set(keys)
            for _ in range(duration): core.retro_run()
            capture(step)
    if state['frame'] is None:
        raise RuntimeError('Aucune image recue')
    output.parent.mkdir(parents=True, exist_ok=True)
    image = to_image()
    image.save(output)
    core.retro_unload_game()
    core.retro_deinit()
    return image.size


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('core', type=Path)
    parser.add_argument('rom', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--frames', type=int, default=180)
    parser.add_argument('--buttons', nargs='*', type=int, default=[])
    args = parser.parse_args()
    print(run(args.core, args.rom, args.output, args.frames, args.buttons))
