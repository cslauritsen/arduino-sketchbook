#!/usr/bin/perl
use strict;


my $pitches = {};


my @pitch_classes = qw( c cs d ds e f fs g gs a as b );
my $lowest_a = 27.50;

# Octaves are reached by doubling or halving any given frequency
# In western music, there are 12 pitch classes each separated by a 'half-step'. 
# The half-step is perceived by he human ear logarithmically. 
#
# The difference in frequency between A and A-sharp is then the frequency of A
# multiplied by the twelfth root of 2.
# If we know the frequency of A, then we can mathematically compute the (equal temperament) frequency of A-sharp thus:
#      a4_freq = 440; // Hz
#      asharp4_freq = 2 * (2**(1/12)) * a4_freq
# (We express the twelfth root of 2 as 2 to its 1/12th power.)
#
# We can easily go outward from here to get the value of any pitch class if we know the number 
# of half steps between it and the reference pitch frequency, by multiplying by the (n/12)th power of 2
# A difference of a major 3rd is 4 half steps. Therefore:
#      n = 4     # number of half-steps between a and c#
#      csharp_freq = (2**(n/12)) * a_freq
#
$pitches ->{'c'}->[0] = $lowest_a * 2 ** (3/12); # c is 3 half-steps higher than a
# initialize all octaves with their A frequency
for (my $i=1; $i < 11; $i++) {
    $pitches->{'c'}->[$i] = $pitches->{'c'}->[$i-1] * 2.0;
}

for (my $octave = 0; $octave < 11; $octave++) {
    for (my $pc=1; $pc < @pitch_classes; $pc++) {
        my $multiplier = 2.0 ** ($pc/12.0);
        $pitches->{$pitch_classes[$pc]}->[$octave] = $pitches->{'c'}->[$octave] * $multiplier;
    }
}

for my $octave (0 .. 10) {
    printf ("\n// Octave %3d\n", $octave+1);;
    for my $pc (@pitch_classes) {
        printf('#define %10s%d  %10.0f', 'P'.uc($pc), $octave+1, ($pitches->{$pc}->[$octave]));
        print "\n";
    }
}
