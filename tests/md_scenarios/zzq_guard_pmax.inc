# (zzq) One image of the guard while the platform X is above `pmax` (variable set by the scenario): `repeat N zzq_guard_pmax.inc` waits (whipping what comes) until the platform comes back down to X <= pmax.
when ram8('vk_platforms+2') > pmax :: include zzq_guard.inc
