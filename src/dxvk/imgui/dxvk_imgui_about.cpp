/*
* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/
#include "dxvk_imgui_about.h"

#include "imgui.h"
#include "dxvk_device.h"
#include "dxvk_context.h"
#include "dxvk_objects.h"
#include "rtx_render/rtx_scenemanager.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dxvk {
  constexpr ImGuiTreeNodeFlags collapsingHeaderClosedFlags = ImGuiTreeNodeFlags_CollapsingHeader;
  constexpr ImGuiTreeNodeFlags collapsingHeaderFlags = collapsingHeaderClosedFlags | ImGuiTreeNodeFlags_DefaultOpen;

  void ImGuiAbout::update(const Rc<DxvkContext>& ctx) {
    m_secrets.update(ctx);
  }

  void ImGuiAbout::show(const Rc<DxvkContext>& ctx) {
    ImGui::PushItemWidth(250);
    ImGui::Text("NVIDIA Lightspeed Studios");
    if (ImGui::CollapsingHeader("Credits", collapsingHeaderFlags)) {
      m_credits.show();
    }
    if (ImGui::CollapsingHeader("Secrets", collapsingHeaderClosedFlags)) {
      m_secrets.show(ctx);
    }
    ImGui::PopItemWidth();
  }

  ImGuiAbout::Credits::Credits()
    : m_sections({
      { "Engineering",
        { "Riley Alston",
          "Xiangshun Bei",
          "Damien Bataille",
          "Alex Dunn",
          "Nicholas Freybler",
          "Mark Henderson",
          "Alexander Jaus",
          "Peter Kristof",
          "Zachary Kupu",
          "Dmitriy Marshak",
          "Yaobin Ouyang",
          "Alexey Panteleev",
          "Sascha Sertel",
          "Nuno Subtil",
          "Ilya Terentiev",
          "Pierre-Olivier Trottier",
          "Lakshmi Vengesanam"}},
      { "Art",
        { "Vernon Andres-Quentin",
          "Filippo Baraccani",
          "Kelsey Blanton",
          "Stan Brown",
          "Rafael Chies",
          "Derk Elshof",
          "Ivan Filipchenko",
          "Hunter Hazen",
          "Fred Hooper",
          "Vadym Kovalenko",
          "Max Kozhevnikov",
          "Gabriele Leone",
          "Evgeny Leonov",
          "Emmanuel Marshall",
          "Aleksey Semenov",
          "Ilya Shelementsev",
          "Dmytro Siromakha",
          "Oleksandr Smirnov",
          "Mostafa Sobhi",
          "Chase Telegin",
          "Oleksii Tronchuk"}},
      { "Production",
        { "Kelsey Blanton",
          "Jaakko Haapasalo",
          "Nyle Usmani"}},
      { "PR/Marketing",
        { "Tim Adams",
          "Brian Burke",
          "Andrew Iain Burnes",
          "Dave Janssen",
          "Jessie Lawrence",
          "Randy Niu",
          "Mike Pepe",
          "Mark Religioso",
          "Kris Rey",
          "Suroosh Taeb",
          "Chris Turner",
          "Keoki Young",
          "Jakob Zamora"}},
      { "Special Thanks",
        { "Alex Hyder",
          "Keith Li",
          "Jarvis McGee",
          "Liam Middlebrook",
          "Adam Moss",
          "Jason Paul",
          "Seth Schneider",
          "Mike Songy",
          "John Spitzer",
          "Sylvain Trottier",
          "--",
          "Everyone contributing to #ct-lss-classic-rtx",
          "Valve"}},
      { "In Memory",
        { "Landon Montgomery" }}
    }) {}

  void ImGuiAbout::Credits::show() {
    for (const auto& creditSection : m_sections) {
      ImGui::Text(creditSection.sectionName);
      ImGui::Indent();
      for (const auto& name : creditSection.names) {
        ImGui::Text(name);
      }
      ImGui::Unindent();
    }
  }

  void ImGuiAbout::Secrets::update(const Rc<DxvkContext>& ctx) {
    auto& iAssetReplacer =
      ctx->getCommonObjects()->getSceneManager().getAssetReplacer();
    if (iAssetReplacer->hasNewSecretReplacementInfo()) {
      const auto& secretReplacements = iAssetReplacer->getSecretReplacementInfo();
      m_organizedSecrets.clear();
      m_codeHashesToSecretPtrs.clear();
      m_assetHashesToSecretPtrs.clear();
      m_visibleHeaders.clear();
      m_validCodeHashesEntered.clear();
      for(auto [hash, secretReplacements] : secretReplacements) {
        for(auto& secretReplacement : secretReplacements) {
          m_organizedSecrets[secretReplacement.header].push_back(Secret{
            secretReplacement,
            false,
            (m_validCodeHashesEntered.count(secretReplacement.unlockHash) > 0)});
        }
      }
      for (auto& [header, secrets] : m_organizedSecrets) {
        m_visibleHeaders[header] = false;
        for (auto& secret : secrets) {
          if (secret.bUnlocked || secret.replacement.bDisplayBeforeUnlocked) {
            m_visibleHeaders[header] = true;
          }
          m_codeHashesToSecretPtrs[secret.replacement.unlockHash].push_back(&secret);
          m_assetHashesToSecretPtrs[secret.replacement.assetHash].push_back(&secret);
        }
      }
    }
    for(const auto validCodeHash : m_validCodeHashesEntered) {
      for(const auto* pSecret : m_codeHashesToSecretPtrs[validCodeHash]) {
      }
    }
  }

  void ImGuiAbout::Secrets::show(const Rc<DxvkContext>& ctx) {
    auto& iAssetReplacer =
      ctx->getCommonObjects()->getSceneManager().getAssetReplacer();
    showCodeHashEntry();
    for (auto& [header, secrets] : m_organizedSecrets) {
      if (m_visibleHeaders[header]) {
        ImGui::Indent();
        if (ImGui::CollapsingHeader(header.c_str(), collapsingHeaderFlags)) {
          for (auto& secret : secrets) {
            if (secret.bUnlocked) {
              if(ImGui::Checkbox(secret.replacement.name.c_str(), &secret.bEnabled)) {
                if(secret.bEnabled && secret.replacement.bExclusiveReplacement) {
                  for(auto* const pOtherSecret : m_assetHashesToSecretPtrs[secret.replacement.assetHash]) {
                    pOtherSecret->bEnabled = (&secret == pOtherSecret);
                  }
                }
                iAssetReplacer->markVariantStatus(secret.replacement.assetHash,
                                                  secret.replacement.variantId,
                                                  secret.bEnabled);
              }
            } else if (secret.replacement.bDisplayBeforeUnlocked) {
              ImGui::Indent();
              ImGui::Text(secret.replacement.name.c_str());
              ImGui::Unindent();
            }
          }
        }
        ImGui::Unindent();
      }
    }
  }

  void ImGuiAbout::Secrets::showCodeHashEntry() {
    ImGui::Text("Codeword:");
    ImGui::SameLine();
    static char codewordBuf[32] = "";
    static auto sameLineAndButton = [&]() {
      ImGui::SameLine();
      return ImGui::Button("Enter");
    };
    if (ImGui::InputText(" " /*Cannot be empty string or else weird ImGUI assert*/,
                          codewordBuf,
                          IM_ARRAYSIZE(codewordBuf),
                          ImGuiInputTextFlags_EnterReturnsTrue)
        || sameLineAndButton()) {
      const XXH64_hash_t hashedCodeword = XXH3_64bits(&codewordBuf[0], strnlen(codewordBuf,IM_ARRAYSIZE(codewordBuf)));
      if((m_validCodeHashesEntered.count(hashedCodeword) == 0) &&
          (m_codeHashesToSecretPtrs.count(hashedCodeword) > 0)) {
        m_validCodeHashesEntered.insert(hashedCodeword);
        for(auto& pSecret : m_codeHashesToSecretPtrs.at(hashedCodeword)) {
          pSecret->bUnlocked = true;
        }
      }
    }
  }
}
