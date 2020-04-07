use 5.012;
use warnings;
use lib 't'; use MyTest;
use Test::More;
use Test::Catch;

$SIG{PIPE} = 'IGNORE';

catch_run('[uews]');

done_testing();
