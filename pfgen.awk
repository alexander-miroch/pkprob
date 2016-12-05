#!/usr/bin/awk -f

BEGIN { 
	OFS=", "; 
	print "float preflop_odds[][9] = {" 
}

{ 
	print "\t{ " $2,$3,$4,$5,$6,$7,$8,$9 " }," 
} 

END { 
	print "};" 
}
