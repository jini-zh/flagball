# Flagball

Flagball is a mod for [Teeworlds][teeworlds], a 2D multiplayer shooter. In
Flagball, there are two goals, one for the red team, the other is for the blue
team. The team goal is located at the back of the enemy base. There is a ball
represented with either a red or a blue flag. The ball can be picked by a tee,
either by touching it (as in the standard CTF game), or by hooking the flag. The
ball carrier cannot shoot ordinary weapons. Instead, when the ball carrier
presses the shoot button, they throw the ball (flag) in the direction they aim.
The ball may also be passed to other players (including the enemy) directly
through hooking. The ball carrier drops the ball on every damage. When the ball
carrier reaches their goal, they score and die to respawn back at their base. If
the carrier throws the ball into the goal, they score too, but their team get
less points. If the server option `sv_fb_owngoal` is activated, the players
that try to score into their own goals will be punished and the enemy team will
score. If the server option `sv_fb_laser_momentum` is set to a nonzero value,
the laser can be used to kick the ball with a given force.

[teeworlds]: https://teeworlds.com/

In the map, the goals are the first two deathmatch spawn points. Playing with
two balls (two flags) is possible.

Use `scripts/commands.pl` to get the list of server configuration variables and
available rcon commands. Pipe it to `scripts/multimarkdown` (requires Perl5
`Text::MultiMarkdown` library) to get the HTML output. You may find the current
list at [my server][commands].

[commands]: http://jini-zh.org/teeworlds/flagball/commands.html

The original author of Flagball is datag. See [the thread on
teeworlds.com][flagball-thread].

[flagball-thread]: https://www.teeworlds.com/forum/viewtopic.php?id=1843

### Random map rotation

Random map rotation makes maps appear in a random sequence. It is activated by
assigning the following value to `sv_maprotation`:

    sv_maprotation "!random flagball.mrt"

Where `flagball.mrt` is a plain text file made of two columns: map name and map
[weight][weight], for example:
    
    fb_sandstorm 0.4
    fb_skyways   0.35
    fb_sol       0.22
    fb_jungle    0.17

Weights are normalized to obtain probabilities.

[weight]: http://en.wikipedia.org/wiki/Weighting
