#include "random_vector.h"
#include <cstdlib>
#include <iostream>

namespace {
double toUnitInterval() {
  return static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
}
}

RandomVector::RandomVector(int size, double max_val) {
  if (size < 0) {
    size = 0;
  }

  vect.resize(size);
  for (int i = 0; i < size; ++i) {
    vect.at(i) = toUnitInterval() * max_val;
  }
}

void RandomVector::print(){
  for (std::size_t i = 0; i < vect.size(); ++i) {
    std::cout << vect.at(i);
    if (i + 1 < vect.size()) {
      std::cout << " ";
    }
  }
  std::cout << std::endl;
}

double RandomVector::mean(){
  if (vect.empty()) {
    return 0.0;
  }

  double sum = 0.0;
  for (std::size_t i = 0; i < vect.size(); ++i) {
    sum += vect.at(i);
  }
  return sum / static_cast<double>(vect.size());
}

double RandomVector::max(){
  if (vect.empty()) {
    return 0.0;
  }

  double result = vect.at(0);
  for (std::size_t i = 1; i < vect.size(); ++i) {
    if (vect.at(i) > result) {
      result = vect.at(i);
    }
  }
  return result;
}

double RandomVector::min(){
  if (vect.empty()) {
    return 0.0;
  }

  double result = vect.at(0);
  for (std::size_t i = 1; i < vect.size(); ++i) {
    if (vect.at(i) < result) {
      result = vect.at(i);
    }
  }
  return result;
}

void RandomVector::printHistogram(int bins){
  if (bins <= 0 || vect.empty()) {
    return;
  }

  std::vector<int> histogram(static_cast<std::size_t>(bins), 0);
  const double lower = min();
  const double upper = max();

  if (lower == upper) {
    histogram.at(0) = static_cast<int>(vect.size());
  } else {
    for (std::size_t i = 0; i < vect.size(); ++i) {
      int index = static_cast<int>(
          ((vect.at(i) - lower) / (upper - lower)) * bins);
      if (index >= bins) {
        index = bins - 1;
      }
      if (index < 0) {
        index = 0;
      }
      ++histogram.at(static_cast<std::size_t>(index));
    }
  }

  int highest = histogram.at(0);
  for (int i = 1; i < bins; ++i) {
    if (histogram.at(static_cast<std::size_t>(i)) > highest) {
      highest = histogram.at(static_cast<std::size_t>(i));
    }
  }

  for (int level = highest; level > 0; --level) {
    for (int bin = 0; bin < bins; ++bin) {
      if (histogram.at(static_cast<std::size_t>(bin)) >= level) {
        std::cout << "***";
      } else {
        std::cout << "   ";
      }
      if (bin + 1 < bins) {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
}
