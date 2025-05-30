%%
%% %CopyrightBegin%
%%
%% SPDX-License-Identifier: Apache-2.0
%%
%% Copyright Ericsson AB 2008-2025. All Rights Reserved.
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

%%

-ifndef(public_key).
-define(public_key, true).

-include("OTP-PUB-KEY.hrl").
-include("PKCS-FRAME.hrl").

-record('SubjectPublicKeyInfoAlgorithm', {
 	  algorithm, 
 	  parameters = asn1_NOVALUE}).

-define(DEFAULT_VERIFYFUN,
	{fun(_,{bad_cert, _} = Reason, _) ->
		 {fail, Reason};
	    (_,{extension, _}, UserState) ->
		 {unknown, UserState};
	    (_, valid, UserState) ->
		 {valid, UserState};
	    (_, valid_peer, UserState) ->
		 {valid, UserState}
	 end, []}).

-record(path_validation_state,
        {
         valid_policy_tree,
         user_initial_policy_set,
         explicit_policy,
         inhibit_any_policy,
         inhibit_policy_mapping,
         policy_mapping_ext,
         policy_constraint_ext,
         policy_inhibitany_ext,
         policy_ext_present,
         policy_ext_any,
         current_any_policy_qualifiers,
         cert_num,
         last_cert = false,
         permitted_subtrees = no_constraints, %% Name constraints
         excluded_subtrees = [],      %% Name constraints
         working_public_key_algorithm,
         working_public_key,
         working_public_key_parameters,
         working_issuer_name,
         max_path_length,
         verify_fun,
         user_state
        }).

-record(revoke_state,
        {
         reasons_mask,
         cert_status,
         interim_reasons_mask,
         valid_ext,
         details
        }).

-record('ECPoint',
        {
         point
        }).

-record(cert,
        {
         der :: public_key:der_encoded(),
         otp :: #'OTPCertificate'{}
        }).

-define(unspecified, 0).
-define(keyCompromise, 1).
-define(cACompromise, 2).
-define(affiliationChanged, 3).
-define(superseded, 4).
-define(cessationOfOperation, 5).
-define(certificateHold, 6).
-define(removeFromCRL, 8).
-define(privilegeWithdrawn, 9).
-define(aACompromise, 10).

-endif. % -ifdef(public_key).
