%%
%% %CopyrightBegin%
%%
%% SPDX-License-Identifier: Apache-2.0
%%
%% Copyright Ericsson AB 1997-2025. All Rights Reserved.
%%
%% Licensed under the Apache License, Version 2.0 (the "License");
%% you may not use this file except in compliance with the License.
%% You may obtain a copy of the License at
%%
%%     http://www.apache.org/licenses/LICENSE-2.0
%%
%% Unless required by applicable law or agreed to in writing, software
%% distributed under the License is distributed on an "AS IS" BASIS,
%% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
%% See the License for the specific language governing permissions and
%% limitations under the License.
%%
%% %CopyrightEnd%
%%
-module(cleanup).

-export([all/0,groups/0,init_per_group/2,end_per_group/2, cleanup/1]).

-include_lib("common_test/include/ct.hrl").

all() -> 
    [cleanup].

groups() -> 
    [].

init_per_group(_GroupName, Config) ->
    Config.

end_per_group(_GroupName, Config) ->
    Config.


cleanup(_) ->
    Localhost = list_to_atom(net_adm:localhost()),
    net_adm:world_list([Localhost]),
    case nodes() of
	[] ->
	    ok;
	Nodes when is_list(Nodes) ->
	    Kill = fun(Node) -> spawn(Node, erlang, halt, []) end,
	    lists:foreach(Kill, Nodes),
	    ct:fail({nodes_left, Nodes})
    end.
