#!/bin/bash

set -euo pipefail

# check programs
type gawk >/dev/null 2>&1 || {
	echo >&2 "'gawk' is not installed!"
	exit 1
}

echo -e "*** WEAPONS ***\n"

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

	print $fields["id"], pen, pen_factor, damage, round(total_damage), round(fire_freq), round(dps), round(dpm), round(tdps), round(tdpm)
}
' stats/weapons.tsv | column -t -s $'\t'

echo -e "\n*** MONSTERS ***\n"

# monster report
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

	print "id", "xph1", "xph10", "h1", "h10", "d1", "d10", "dps1", "dps10"
}
NR > 1 && $fields["id"] ~ /^monster|boss/ {

	h1 = $fields["health"]
	h10 = h1 + ($fields["health_level"] - 1) * 10
	xph1 = $fields["xp"] / h1
	xph10 = ($fields["xp"] + ($fields["xp_level"] - 1) * 10) / h10
	damage1 = $fields["damage"]
	damage10 = damage1 + ($fields["damage_level"] - 1) * 10
	attack_period = $fields["attack_period"]
	dps1 = damage1 / attack_period
	dps10 = damage10 / attack_period

	print $fields["id"], round(xph1), round(xph10), h1, h10, damage1, damage10, round(dps1), round(dps10)
}
' stats/monsters.tsv | column -t -s $'\t'
