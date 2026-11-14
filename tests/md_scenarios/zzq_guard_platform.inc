# (zzq) Stage 10 room 0: one image of the guard while the platform has not reached its right end (X 186); nothing (no image) once it has, so
# `repeat N zzq_guard_platform.inc` is a wait "until the platform is at its maximum" during which the bats are whipped.
when ram8('vk_platforms+2') < 186 :: include zzq_guard.inc
