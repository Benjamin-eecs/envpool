# Copyright 2022 Garena Online Private Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Mujoco env in EnvPool."""

from envpool.python.api import py_env

from .mujoco_envpool import (
  _AntEnvPool,
  _AntEnvSpec,
  _HalfCheetahEnvPool,
  _HalfCheetahEnvSpec,
  _HopperEnvPool,
  _HopperEnvSpec,
  _HumanoidEnvPool,
  _HumanoidEnvSpec,
)

AntEnvSpec, AntDMEnvPool, AntGymEnvPool = py_env(_AntEnvSpec, _AntEnvPool)

HalfCheetahEnvSpec, HalfCheetahDMEnvPool, HalfCheetahGymEnvPool = py_env(
  _HalfCheetahEnvSpec, _HalfCheetahEnvPool
)

HopperEnvSpec, HopperDMEnvPool, HopperGymEnvPool = py_env(
  _HopperEnvSpec, _HopperEnvPool
)

HumanoidEnvSpec, HumanoidDMEnvPool, HumanoidGymEnvPool = py_env(
  _HumanoidEnvSpec, _HumanoidEnvPool
)

__all__ = [
  "AntEnvSpec",
  "AntDMEnvPool",
  "AntGymEnvPool",
  "HalfCheetahEnvSpec",
  "HalfCheetahDMEnvPool",
  "HalfCheetahGymEnvPool",
  "HopperEnvSpec",
  "HopperDMEnvPool",
  "HopperGymEnvPool",
  "HumanoidEnvSpec",
  "HumanoidDMEnvPool",
  "HumanoidGymEnvPool",
]