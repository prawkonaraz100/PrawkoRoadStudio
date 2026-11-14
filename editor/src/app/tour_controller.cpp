/*
 * Copyright 2026 Robomous
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "app/tour_controller.hpp"

#include <QObject>
#include <utility>

namespace roadmaker::editor {

TourController::TourController(std::vector<TourStep> steps) : steps_(std::move(steps)) {}

void TourController::start() {
  index_ = 0;
  completed_ = false;
  if (steps_.empty()) {
    active_ = false;
    completed_ = true;
    return;
  }
  active_ = true;
}

const TourStep* TourController::current() const {
  if (!active_ || index_ >= steps_.size()) {
    return nullptr;
  }
  return &steps_[index_];
}

void TourController::next() {
  if (!active_) {
    return;
  }
  if (index_ + 1 >= steps_.size()) {
    active_ = false;
    completed_ = true;
    return;
  }
  ++index_;
}

void TourController::skip() {
  if (!active_) {
    return;
  }
  active_ = false;
  completed_ = true;
}

std::vector<TourStep> default_tour_steps() {
  return {
      TourStep{QObject::tr("Narysuj drogę"),
               QObject::tr("Wybierz Droga, wskaż punkty przebiegu na planszy i naciśnij Enter. "
                           "Możesz też przeciągnąć gotową drogę z naszej biblioteki."),
               QStringLiteral("Droga")},
      TourStep{QObject::tr("Trzy kategorie Prawko"),
               QObject::tr("Kliknij Dodaj: wybierz Gotowe drogi, Krawężniki i chodniki albo "
                           "Uczestnicy ruchu. Przeciągnij komponent z biblioteki na planszę."),
               QStringLiteral("Dodaj")},
      TourStep{QObject::tr("Ustaw pozycję"),
               QObject::tr("Zaznacz element kliknięciem, a następnie przesuń go myszą. "
                           "Cofnij przywraca ostatnią zmianę."),
               QStringLiteral("Przesuń")},
      TourStep{QObject::tr("Zaplanuj ruch"),
               QObject::tr("Użyj trybu Scenariusz i narzędzia Trasa, aby nadać pojazdowi "
                           "lub pieszemu ruch na planszy."),
               QStringLiteral("Scenariusz")},
      TourStep{QObject::tr("Uruchom scenę"),
               QObject::tr("Po dodaniu uczestników i ich tras kliknij Symulacja, "
                           "aby obliczyć przebieg i odtworzyć ruch."),
               QStringLiteral("Symulacja")},
  };
}

} // namespace roadmaker::editor
