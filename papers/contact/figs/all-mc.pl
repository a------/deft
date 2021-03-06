#!/usr/bin/perl -w

use strict;
use Digest::SHA1 qw(sha1_hex);
use File::Slurp qw(read_file);

my $dir = "papers/contact/figs";
my $sha1code = sha1_hex(read_file("src/Monte-Carlo/monte-carlo.cpp"));

system "make monte-carlo";

my $iters  = 99999999999999;
my $acc = 0.001;

my $dd;
foreach $dd ([16,430], [16,1880]) {
    my $radius = sprintf("%02.0f", $$dd[0]);
    my $N = sprintf("%03.0f", $$dd[1]);

    # Here I estimate the amount of memory that will be needed...
    my $memuse = sprintf "%.0f", 0.001*($N) + 30; # It's a very hokey guess

    my $scriptname = "papers/contact/figs/mc-sphere-$radius-$N.tmp.sh";

    my $outfilename = "$dir/mc-$radius-$N.dat";
    my $command = "./monte-carlo $N $iters $acc $outfilename outerSphere $radius";

    open SCRIPT, ">$scriptname" or die $!;
    print SCRIPT "#!/bin/sh
#SBATCH --mem-per-cpu=$memuse
##SBATCH --mail-type ALL
##SBATCH --mail-user jeffschulte83\@gmail.com
#SBATCH --output mc-sphere-$radius-$N.out

set -ev

hostname
date

echo I think this will take $memuse megs of memory
time nice -19 $command

cat > $outfilename.sum << ENDF
$sha1code
$command
ENDF

date

";
    close(SCRIPT);
    my $oldsha1 = "\n";
    if (-e "$outfilename.sum") {
      $oldsha1 = read_file("$outfilename.sum");
    }
    if ($oldsha1 eq "$sha1code\n$command\n") {
      print "Monte-carlo in cavity with radius $radius has already been done.\n"
    } else {
      print "Submitting monte-carlo in cavity with radius $radius\n";

      system("sbatch", $scriptname);
    }
}

foreach $dd ([20,196], [20,817], [24,1320]) {
    my $len = sprintf("%02.0f", $$dd[0]);
    my $N = sprintf("%03.0f", $$dd[1]);

    # Here I estimate the amount of memory that will be needed...
    my $memuse = sprintf "%.0f", 0.001*($N) + 30; # It's a very hokey guess

    my $scriptname = "papers/contact/figs/mc-walls-$len-$N.tmp.sh";

    my $outfilename = "$dir/mc-walls-$len-$N.dat";
    my $command = "./monte-carlo $N $iters $acc $outfilename periodx $len periody $len wallz $len flatdiv";

    open SCRIPT, ">$scriptname" or die $!;
    print SCRIPT "#!/bin/sh
#SBATCH --mem-per-cpu=$memuse
##SBATCH --mail-type ALL
##SBATCH --mail-user jeffschulte83\@gmail.com
#SBATCH --output mc-walls-$len-$N.out

set -ev

hostname
date

echo I think this will take $memuse megs of memory
time nice -19 $command

cat > $outfilename.sum << ENDF
$sha1code
$command
ENDF

date

";
    close(SCRIPT);
    my $oldsha1 = "\n";
    if (-e "$outfilename.sum") {
      $oldsha1 = read_file("$outfilename.sum");
    }
    if ($oldsha1 eq "$sha1code\n$command\n") {
      print "Monte-carlo in cubic cavity with length $len wallz and flat divisions has already been done.\n"
    } else {
      print "Submitting monte-carlo in cubic cavity with length $len wallz and flat divisions\n";

      system("sbatch", $scriptname);
    }
}



foreach $dd ( [2,24,1647],[2,24,1320],[6,32,764], [6,32,3070], [8,40,1493], [8,40,5947],[8,40,5750]){

    my $innerRad = sprintf("%02.0f", $$dd[0]);
    my $len = sprintf("%02.0f", $$dd[1]);
    my $N = sprintf("%03.0f", $$dd[2]);

    # Here I estimate the amount of memory that will be needed...
    my $memuse = sprintf "%.0f", 0.001*($N) + 30; # It's a very hokey guess

    my $scriptname = "papers/contact/figs/mc-periodic-innerSphere-$len-$innerRad-$N.tmp.sh";

    my $outfilename = "$dir/mc-innerSphere-$len-$innerRad-$N.dat";
    my $command = "./monte-carlo $N $iters $acc $outfilename innerSphere $innerRad periodx $len periody $len periodz $len";

    open SCRIPT, ">$scriptname" or die $!;
    print SCRIPT "#!/bin/sh
#SBATCH --mem-per-cpu=$memuse
##SBATCH --mail-type ALL
##SBATCH --mail-user jeffschulte83\@gmail.com
#SBATCH --output mc-innerSphere-$len-$innerRad-$N.out

set -ev

hostname
date

echo I think this will take $memuse megs of memory
time nice -19 $command

cat > $outfilename.sum << ENDF
$sha1code
$command
ENDF

date

";
    close(SCRIPT);
    my $oldsha1 = "\n";
    if (-e "$outfilename.sum") {
      $oldsha1 = read_file("$outfilename.sum");
    }
    if ($oldsha1 eq "$sha1code\n$command\n") {
      print "Monte-carlo in cubic cavity with inner sphere radius $innerRad has already been done.\n"
    } else {
      print "Submitting monte-carlo in cubic cavity with inner sphere radius $innerRad\n";

      system("sbatch", $scriptname);
    }
}

