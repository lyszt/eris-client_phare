%% eris_router.pl - Logical command router for Eris CLI
%% Supports: and / or / not, multiparameter and prefix commands.
%% (Word connectives, not && / ||, to avoid the shell intercepting them.)
%% Loaded from C via consult/1; predicates are in user module.
%%
%% eris_goals(ArgvList, GoalList)
%%   ArgvList: list of atoms from C argv (e.g. [eris, alias, add, test, do, echo, hello])
%%   GoalList: list of goal(Op, Command)
%%     Op = and | or  (connective to previous goal)
%%     Command = help | init(Params) | commit(Params) | ... | not(Inner) | alias_add(ArgList)
%%   C executes the chain: and = run and continue only on success; or = run and stop on success.

:- multifile command_usage/3.  % (Canonical, Aliases, ParamSpec) from each module .pl

%% --- Main entry: parse argv into list of goals ---
%% Throws error(eris_syntax(Message)) for user-facing syntax errors (e.g. unknown command).
%% Drop the program name once, up front (the user writes it only once:
%% `eris init and commit`), then split the remainder on the connectives.
%% `alias add` stores a command chain, so its tokens keep the connectives verbatim
%% instead of being split into goals.
eris_goals(Argv, GoalList) :-
    drop_program_name(Argv, Rest),
    (   Rest = [alias, add|Args]
    ->  GoalList = [goal(and, alias_add(Args))]
    ;   split_by_connectives(Rest, Segs),
        maplist(seg_to_goal, Segs, GoalList)
    ).

%% Split at 'and' and 'or'. Each segment is (Op, Tokens); Op is the connective
%% that precedes this segment (and/or). First segment gets 'and'.
split_by_connectives(Argv, Segs) :-
    split_at_connectives(Argv, Segs).

split_at_connectives([], []).
split_at_connectives(Argv, [(Op, Seg)|Rest]) :-
    take_until_connective(Argv, Seg, Conn, Tail),
    Seg \= [],
    ( Conn = end -> Rest = [], Op = and
    ; Conn = and -> Op = and, split_at_connectives(Tail, Rest)
    ; Conn = or  -> Op = or,  split_at_connectives(Tail, Rest)
    ),
    !.
split_at_connectives(Argv, [(and, Argv)]) :- Argv \= [].

take_until_connective([], [], end, []).
take_until_connective([and|T], [], and, T).
take_until_connective([or|T], [], or, T).
take_until_connective([H|T], [H|Seg], Conn, Tail) :-
    H \= and, H \= or,
    take_until_connective(T, Seg, Conn, Tail).

seg_to_goal((Op, Toks), goal(Op, Cmd)) :-
    parse_command(Toks, Cmd).

drop_program_name([], []).
drop_program_name([_|T], T).  % drop first token (program name, e.g. eris)

%% take_params(ParamSpec, Params, Taken): consume tokens according to module spec.
%% [optional] = 0 or 1 arg (e.g. init); missing param fallback is applied in C (e.g. init → folder name).
take_params([], _, []).
take_params([optional], Params, Taken) :- ( Params = [P|_] -> Taken = [P] ; Taken = [] ).
take_params([rest], Params, Params).

%% Parse one segment: optional leading 'not', then command (with params from command_usage).
parse_command([], help).
parse_command([not|Rest], not(Inner)) :-
    parse_command(Rest, Inner),
    !.
parse_command([alias, add|Args], alias_add(Args)) :- !.
parse_command([alias, run|Args], alias_run(Args)) :- !.
parse_command([run|Args], alias_run(Args)) :- !.  % shorthand: eris run <name>
parse_command([alias|_], help) :- !.
parse_command([Cmd|Params], Command) :-
    command_usage(Canonical, Aliases, Spec),
    member(Cmd, Aliases),
    take_params(Spec, Params, Taken),
    Command =.. [Canonical, Taken],
    !.
parse_command([Cmd|_], _) :-
    atom(Cmd),
    format(atom(M), 'Unknown command: ~w', [Cmd]),
    throw(error(eris_syntax(M))).
parse_command([_|_], help).  % fallback

%% Single-command lookup: returns functor atom for C (init, commit, help, ...).
eris_command(Argv, Command) :-
    eris_goals(Argv, [goal(_, Cmd)|_]),
    ( Cmd = not(_)   -> Command = help
    ; Cmd = alias_add(_) -> Command = help
    ; Cmd = alias_run(_) -> Command = help
    ; Cmd =.. [Command|_]  % compound: init([]), commit([]), etc.
    ),
    !.
eris_command(Argv, help) :-
    eris_goals(Argv, []),
    !.
eris_command(_, help).
