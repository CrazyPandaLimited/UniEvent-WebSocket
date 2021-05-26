use 5.012;
use warnings;
use lib 't'; use MyTest;
use Test::More;
use Test::Catch;

$SIG{PIPE} = 'IGNORE';

test_catch('[uews]');

done_testing();
