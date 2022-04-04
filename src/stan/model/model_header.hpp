#ifndef STAN_MODEL_MODEL_HEADER_HPP
#define STAN_MODEL_MODEL_HEADER_HPP

#include <stan/math.hpp>

#include <stan/io/cmd_line.hpp>
#include <stan/io/dump.hpp>
#include <stan/io/deserializer.hpp>
#include <stan/io/serializer.hpp>
#include <stan/io/reader.hpp>
#include <stan/io/writer.hpp>

#include <stan/model/rethrow_located.hpp>
#include <stan/model/model_base.hpp>
#include <stan/model/model_base_crtp.hpp>
#include <stan/model/indexing.hpp>
#include <stan/services/util/create_rng.hpp>

#include <boost/random/additive_combine.hpp>
#include <boost/random/linear_congruential.hpp>

#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#endif
