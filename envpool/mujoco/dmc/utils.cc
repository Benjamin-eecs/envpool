// Copyright 2022 Garena Online Private Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "envpool/mujoco/dmc/utils.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "pugixml.hpp"

namespace mujoco_dmc {

std::string GetFileContent(const std::string& base_path,
                           const std::string& asset_name) {
  // hardcode path here :(
  std::string filename = base_path + "/mujoco/assets_dmc/" + asset_name;
  std::ifstream ifs(filename);
  std::stringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

class XMLStringWriter : public pugi::xml_writer {
 public:
  std::string result;
  void write(const void* data, size_t size) override {
    result.append(static_cast<const char*>(data), size);
  }
};

std::string XMLRemoveByBodyName(const std::string& content,
                                const std::vector<std::string>& body_names) {
  pugi::xml_document doc;
  doc.load_string(content.c_str());
  for (const auto& name : body_names) {
    std::string xpath = "//body[@name='" + name + "']";
    pugi::xml_node node = doc.select_node(xpath.c_str()).node();
    auto parent = node.parent();
    parent.remove_child(node);
  }
  XMLStringWriter writer;
  doc.print(writer);
  return writer.result;
}

std::string XMLAddPoles(const std::string& content, int pole_numbers) {
  pugi::xml_document doc;
  doc.load_string(content.c_str());

  if (pole_numbers != 1) {
    //   parent = mjcf.find('./worldbody/body/body')  # Find first pole.
    std::string xpath = "/worldbody/body/body[@name=pole_1]";
    pugi::xml_node parent = doc.select_node(xpath.c_str()).node();
    for (int i = 0; i < n_poles_; ++i) {
      // add poles not implemented
      // child = etree.Element('body', name='pole_{}'.format(pole_index),
      // pos='0 0 1', childclass='pole')
      pugi::xml_node childpole = parent.append_child("body");
      childpole.append_attribute("name") = "pole" + ToString(i);
      childpole.append_attribute("pos") = "0 0 1";
      childpole.append_attribute("childclass") = "pole";
      // etree.SubElement(child, 'joint', name='hinge_{}'.format(pole_index))
      pugi::xml_node sub1 = childpole.append_child("joint");
      sub1.append_attribute("name") = "hinge" + ToString(i);
      // etree.SubElement(child, 'geom', name='pole_{}'.format(pole_index))
      pugi::xml_node sub1 = childpole.append_child("joint");
      sub1.append_attribute("geom") = "pole" + ToString(i);
      // parent.append(child)
      // parent = child
      parent = childpole;
      return;
    }
    // Move plane down.
    // floor = mjcf.find('./worldbody/geom')
    // floor.set('pos', '0 0 {}'.format(1 - n_poles - .05))
    std::string floor_xpath = "/worldbody/geom";
    pugi::xml_node floor = doc.select_node(floor_xpath.c_str()).node();
    pugi::xml_attribute attr = floor.attribute("pos");
    attr.set_value("0 " + ToString(1 - n_poles_ - 0.05) + " 1");
    // Move cameras back.
    // cameras = mjcf.findall('./worldbody/camera')
    std::string camera0_xpath = "/worldbody/camera[@name=fixed]";
    std::string camera1_xpath = "/worldbody/camera[@name=lookatcart]";
    // cameras[0].set('pos', '0 {} 1'.format(-1 - 2*n_poles))
    pugi::xml_node camera0 = doc.select_node(camera0_xpath.c_str()).node();
    pugi::xml_attribute camera0_attr = camera0.attribute("pos");
    camera0_attr.set_value("0 " + ToString(-1 - 2 * n_poles_) + " 1")
        // cameras[1].set('pos', '0 {} 2'.format(-2*n_poles))
        pugi::xml_node camera1 = doc.select_node(camera1_xpath.c_str()).node();
    pugi::xml_attribute camera1_attr = camera1.attribute("pos");
    camera1_attr.set_value("0 " + ToString(-1 - 2 * n_poles_) + " 1");
  }
  XMLStringWriter writer;
  doc.print(writer);
  return writer.result;
}

int GetQposId(mjModel* model, const std::string& name) {
  return model->jnt_qposadr[mj_name2id(model, mjOBJ_JOINT, name.c_str())];
}

int GetQvelId(mjModel* model, const std::string& name) {
  return model->jnt_dofadr[mj_name2id(model, mjOBJ_JOINT, name.c_str())];
}

int GetSensorId(mjModel* model, const std::string& name) {
  return model->sensor_adr[mj_name2id(model, mjOBJ_SENSOR, name.c_str())];
}

// rewards
double RewardTolerance(double x, double bound_min, double bound_max,
                       double margin, double value_at_margin,
                       SigmoidType sigmoid_type) {
  if (bound_min <= x && x <= bound_max) {
    return 1.0;
  }
  if (margin <= 0.0) {
    return 0.0;
  }
  x = (x < bound_min ? bound_min - x : x - bound_max) / margin;
  if (sigmoid_type == SigmoidType::kGaussian) {
    // scale = np.sqrt(-2 * np.log(value_at_1))
    // return np.exp(-0.5 * (x*scale)**2)
    double scaled_x = std::sqrt(-2 * std::log(value_at_margin)) * x;
    return std::exp(-0.5 * scaled_x * scaled_x);
  }
  if (sigmoid_type == SigmoidType::kHyperbolic) {
    // scale = np.arccosh(1/value_at_1)
    // return 1 / np.cosh(x*scale)
    double scaled_x = std::acosh(1 / value_at_margin) * x;
    return 1 / std::cosh(scaled_x);
  }
  if (sigmoid_type == SigmoidType::kLongTail) {
    // scale = np.sqrt(1/value_at_1 - 1)
    // return 1 / ((x*scale)**2 + 1)
    double scaled_x = std::sqrt(1 / value_at_margin - 1) * x;
    return 1 / (scaled_x * scaled_x + 1);
  }
  if (sigmoid_type == SigmoidType::kReciprocal) {
    // scale = 1/value_at_1 - 1
    // return 1 / (abs(x)*scale + 1)
    double scale = 1 / value_at_margin - 1;
    return 1 / (std::abs(x) * scale + 1);
  }
  if (sigmoid_type == SigmoidType::kCosine) {
    // scale = np.arccos(2*value_at_1 - 1) / np.pi
    // scaled_x = x*scale
    // with warnings.catch_warnings():
    //   warnings.filterwarnings(
    //       action='ignore', message='invalid value encountered in cos')
    //   cos_pi_scaled_x = np.cos(np.pi*scaled_x)
    // return np.where(abs(scaled_x) < 1, (1 + cos_pi_scaled_x)/2, 0.0)
    const double pi = std::acos(-1);
    double scaled_x = std::acos(2 * value_at_margin - 1) / pi * x;
    return std::abs(scaled_x) < 1 ? (1 + std::cos(pi * scaled_x)) / 2 : 0.0;
  }
  if (sigmoid_type == SigmoidType::kLinear) {
    // scale = 1-value_at_1
    // scaled_x = x*scale
    // return np.where(abs(scaled_x) < 1, 1 - scaled_x, 0.0)
    double scaled_x = (1 - value_at_margin) * x;
    return std::abs(scaled_x) < 1 ? 1 - scaled_x : 0.0;
  }
  if (sigmoid_type == SigmoidType::kQuadratic) {
    // scale = np.sqrt(1-value_at_1)
    // scaled_x = x*scale
    // return np.where(abs(scaled_x) < 1, 1 - scaled_x**2, 0.0)
    double scaled_x = std::sqrt(1 - value_at_margin) * x;
    return std::abs(scaled_x) < 1 ? 1 - scaled_x * scaled_x : 0.0;
  }
  if (sigmoid_type == SigmoidType::kTanhSquared) {
    // scale = np.arctanh(np.sqrt(1-value_at_1))
    // return 1 - np.tanh(x*scale)**2
    double scaled_x = std::atanh(std::sqrt(1 - value_at_margin)) * x;
    return 1 - std::tanh(scaled_x) * std::tanh(scaled_x);
  }
  throw std::runtime_error("Unknown sigmoid type for RewardTolerance.");
}

}  // namespace mujoco_dmc
