#!/bin/bash

# check programs
type gawk >/dev/null 2>&1 || {
	echo >&2 "'gawk' is not installed!"
	exit 1
}

# weapon report
gawk '
function min(x, y) { return x < y ? x : y }
function max(x, y) { return x > y ? x : y }
function round(value) { return int(value * 100) / 100 }

BEGIN {
	FS = OFS = "\t"
}
NR == 1 {
	for(i = 1; i <= NF; i++)
		fields[$i] = i

	print "id", "pen", "pen_factor", "single_damage", "total_damage", "fire_freq", "dps", "dpm", "tdps", "tdpm"
}
NR > 1 {
	fire_period = $fields["fire_period"]
	fire_freq = 1 / fire_period
	attack_count = $fields["attack_count"] 
	rounds = max(1, $fields["rounds"])
	if($fields["fire_allrounds"])
		attack_count *= rounds
	damage = $fields["damage"] * attack_count * max(1, $fields["burst_rounds"])
	pen = $fields["penetration"]
	pen_factor = $fields["penetration_damage"]
	dps = damage / fire_period
	dpm = damage * rounds
	total_damage = damage
	for(i = 1; i < pen; i++)
		total_damage += damage * pen_factor ^ i
	tdpm = total_damage * rounds
	tdps = total_damage / fire_period

	print $fields["id"], pen, pen_factor, damage, total_damage, round(fire_freq), round(dps), dpm, tdps, tdpm
}
' stats/weapons.tsv
