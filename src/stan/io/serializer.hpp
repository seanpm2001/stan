#ifndef STAN_IO_SERIALIZER_HPP
#define STAN_IO_SERIALIZER_HPP

#include <stan/math/rev.hpp>

namespace stan {
namespace io {

/**
 * A stream-based writer for scalar, vector, matrix, and array data types.
 *
 *`T` is the storage scalar type. Variables written by the serializer must
 * have a scalar type convertible to type `T`.
 *
 * @tparam T Basic scalar type.
 */
template <typename T>
class serializer {
 private:
  Eigen::Map<Eigen::Matrix<T, -1, 1>> map_r_;  // map of reals.
  size_t r_size_{0};                           // size of reals available.
  size_t pos_r_{0};  // current position in map of reals.

  /**
   * Check there is room for at least m more reals to store
   *
   * @param m Number of reals to Write
   * @throws std::runtime_error if there isn't room for m reals
   */
  void check_r_capacity(size_t m) const {
    if (pos_r_ + m > r_size_) {
      []() STAN_COLD_PATH {
        throw std::runtime_error("no more storage available to write");
      }();
    }
  }

  template <typename S>
  using is_arithmetic_or_ad
      = bool_constant<std::is_arithmetic<S>::value || is_autodiff<S>::value>;

 public:
  using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using vector_t = Eigen::Matrix<T, Eigen::Dynamic, 1>;
  using row_vector_t = Eigen::Matrix<T, 1, Eigen::Dynamic>;

  using map_matrix_t = Eigen::Map<matrix_t>;
  using map_vector_t = Eigen::Map<vector_t>;
  using map_row_vector_t = Eigen::Map<row_vector_t>;

  using var_matrix_t = stan::math::var_value<
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>;
  using var_vector_t
      = stan::math::var_value<Eigen::Matrix<double, Eigen::Dynamic, 1>>;
  using var_row_vector_t
      = stan::math::var_value<Eigen::Matrix<double, 1, Eigen::Dynamic>>;

  /**
   * Construct a variable serializer using data_r for storage.
   *
   * Attempting to write beyond the end of data_r will raise a runtime
   * exception.
   *
   * @param RVec Vector like class.
   * @param data_r Storage vector
   */
  template <typename RVec, require_vector_like_t<RVec>* = nullptr>
  explicit serializer(RVec& data_r)
      : map_r_(data_r.data(), data_r.size()), r_size_(data_r.size()) {}

  /**
   * Return the number of scalars available to be written to.
   */
  inline size_t available() const noexcept { return r_size_ - pos_r_; }

  /**
   * Write a scalar to storage
   * @tparam Scalar A Stan scalar class
   * @param x A scalar
   */
  template <typename Scalar, require_t<is_arithmetic_or_ad<Scalar>>* = nullptr,
            require_not_var_matrix_t<Scalar>* = nullptr>
  inline void write(Scalar x) {
    check_r_capacity(1);
    map_r_.coeffRef(pos_r_) = x;
    pos_r_++;
  }

  /**
   * Write a complex variable to storage
   * @tparam Complex An `std::complex` type.
   * @param x The complex scalar
   */
  template <typename Complex, require_complex_t<Complex>* = nullptr>
  inline void write(Complex&& x) {
    check_r_capacity(2);
    map_r_.coeffRef(pos_r_) = x.real();
    map_r_.coeffRef(pos_r_ + 1) = x.imag();
    pos_r_ += 2;
  }

  /**
   * Write an Eigen column vector to storage
   * @tparam Vec An Eigen type with compile time columns equal to 1.
   * @param vec The Eigen column vector
   */
  template <typename Vec, require_eigen_col_vector_t<Vec>* = nullptr,
            require_not_vt_complex<Vec>* = nullptr>
  inline void write(Vec&& vec) {
    check_r_capacity(vec.size());
    map_vector_t(&map_r_.coeffRef(pos_r_), vec.size()) = vec;
    pos_r_ += vec.size();
  }

  /**
   * Write a Eigen column vector with inner complex type to storage
   * @tparam Vec An Eigen type with compile time columns equal to 1.
   * @param vec The Eigen column vector
   */
  template <typename Vec, require_eigen_col_vector_t<Vec>* = nullptr,
            require_vt_complex<Vec>* = nullptr>
  inline void write(Vec&& vec) {
    check_r_capacity(2 * vec.size());
    using stan::math::to_ref;
    auto&& vec_ref = to_ref(std::forward<Vec>(vec));
    for (Eigen::Index i = 0; i < vec_ref.size(); ++i) {
      map_r_.coeffRef(pos_r_) = vec_ref.coeff(i).real();
      map_r_.coeffRef(pos_r_ + 1) = vec_ref.coeff(i).imag();
      pos_r_ += 2;
    }
  }

  /**
   * Write an Eigen row vector to storage
   * @tparam Vec An Eigen type with compile time rows equal to 1.
   * @param vec The Eigen row vector
   */
  template <typename Vec, require_eigen_row_vector_t<Vec>* = nullptr,
            require_not_vt_complex<Vec>* = nullptr>
  inline void write(Vec&& vec) {
    check_r_capacity(vec.size());
    map_row_vector_t(&map_r_.coeffRef(pos_r_), vec.size()) = vec;
    pos_r_ += vec.size();
  }

  /**
   * Write an Eigen row vector with inner complex type to storage
   * @tparam Vec An Eigen type with compile time rows equal to 1.
   * @param vec The Eigen row vector
   */
  template <typename Vec, require_eigen_row_vector_t<Vec>* = nullptr,
            require_vt_complex<Vec>* = nullptr>
  inline void write(Vec&& vec) {
    using stan::math::to_ref;
    check_r_capacity(2 * vec.size());
    auto&& vec_ref = to_ref(std::forward<Vec>(vec));
    for (Eigen::Index i = 0; i < vec_ref.size(); ++i) {
      map_r_.coeffRef(pos_r_) = vec_ref.coeff(i).real();
      map_r_.coeffRef(pos_r_ + 1) = vec_ref.coeff(i).imag();
      pos_r_ += 2;
    }
  }

  /**
   * Write a Eigen matrix of size `(rows, cols)` to storage
   * @tparam Mat An Eigen class with dynamic rows and columns
   * @param mat An Eigen object
   */
  template <typename Mat, require_eigen_matrix_dynamic_t<Mat>* = nullptr,
            require_not_vt_complex<Mat>* = nullptr>
  inline void write(Mat&& mat) {
    check_r_capacity(mat.size());
    map_matrix_t(&map_r_.coeffRef(pos_r_), mat.rows(), mat.cols()) = mat;
    pos_r_ += mat.size();
  }

  /**
   * Write a Eigen matrix of size `(rows, cols)` with complex inner type to
   * storage
   * @tparam Mat The type to write
   * @param rows The size of the rows of the matrix.
   * @param cols The size of the cols of the matrix.
   */
  template <typename Mat, require_eigen_matrix_dynamic_t<Mat>* = nullptr,
            require_vt_complex<Mat>* = nullptr>
  inline void write(Mat&& x) {
    check_r_capacity(2 * x.size());
    using stan::math::to_ref;
    auto&& x_ref = to_ref(x);
    for (Eigen::Index i = 0; i < x.size(); ++i) {
      map_r_.coeffRef(pos_r_) = x_ref.coeff(i).real();
      map_r_.coeffRef(pos_r_ + 1) = x_ref.coeff(i).imag();
      pos_r_ += 2;
    }
  }

  /**
   * Write a `var_value` with inner Eigen type to storage
   * @tparam Ret The type to write
   * @tparam T_ Should never be set by user, set to default value of `T` for
   *  performing deduction on the class's inner type.
   *  dimensions of the `var_value` matrix or vector.
   */
  template <typename VarMat, typename T_ = T, require_var_t<T_>* = nullptr,
            require_var_matrix_t<VarMat>* = nullptr>
  inline void write(VarMat&& x) {
    check_r_capacity(x.size());
    map_matrix_t(&map_r_.coeffRef(pos_r_), x.rows(), x.cols())
        = stan::math::from_var_value(x);
    pos_r_ += x.size();
  }

  /**
   * Write an Eigen type when the serializers inner class is not var.
   * @tparam VarMat The type to write
   * @tparam T_ Should never be set by user, set to default value of `T` for
   *  performing deduction on the class's inner type.
   *  dimensions of the `var_value` matrix or vector.
   */
  template <typename VarMat, typename T_ = T, require_not_var_t<T_>* = nullptr,
            require_var_matrix_t<VarMat>* = nullptr>
  inline void write(VarMat&& x) {
    this->write(stan::math::value_of(x));
  }

  /**
   * Write a `std::vector` to storage
   * @tparam StdVec The type to write
   */
  template <typename StdVec, require_std_vector_t<StdVec>* = nullptr>
  inline void write(StdVec&& x) {
    for (auto&& x_i : x) {
      this->write(x_i);
    }
  }


    /**
     * Write a serialized lower bounded variable and unconstrain it
     *
     * @tparam Ret Type of output
     * @tparam L Type of lower bound
     * @tparam Sizes Types of dimensions of output
     * @param lb Lower bound
     * @param sizes dimensions
     */
    template <typename Ret, typename L>
    inline void write_free_lb(const L& lb, const Ret& ret) {
      this->write(stan::math::lb_free(ret, lb));
    }

    /**
     * Write a serialized lower bounded variable and unconstrain it
     *
     * @tparam Ret Type of output
     * @tparam U Type of upper bound
     * @tparam Sizes Types of dimensions of output
     * @param ub Upper bound
     * @param sizes dimensions
     */
    template <typename Ret, typename U>
    inline void write_free_ub(const U& ub, const Ret& ret) {
      this->write(stan::math::ub_free(ret, ub));
    }

    /**
     * Write a serialized bounded variable and unconstrain it
     *
     * @tparam Ret Type of output
     * @tparam L Type of lower bound
     * @tparam U Type of upper bound
     * @tparam Sizes Types of dimensions of output
     * @param lb Lower bound
     * @param ub Upper bound
     * @param sizes dimensions
     */
    template <typename Ret, typename L, typename U>
    inline void write_free_lub(const L& lb, const U& ub, const Ret& ret) {
      this->write(stan::math::lub_free(ret, lb, ub));
    }

    /**
     * Write a serialized offset-multiplied variable and unconstrain it
     *
     * @tparam Ret Type of output
     * @tparam O Type of offset
     * @tparam M Type of multiplier
     * @tparam Sizes Types of dimensions of output
     * @param offset Offset
     * @param multiplier Multiplier
     * @param sizes dimensions
     */
    template <typename Ret, typename O, typename M>
    inline void write_free_offset_multiplier(const O& offset, const M& multiplier,
                                            const Ret& ret) {
      this->write(stan::math::offset_multiplier_free(ret, offset, multiplier));
    }

    /**
     * Write a serialized unit vector and unconstrain it
     *
     * @tparam Ret Type of output
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_unit_vector(const Ret& ret) {
      this->write(stan::math::unit_vector_free(ret));
    }

    /**
     * Write serialized unit vectors and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_unit_vector(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_unit_vector(ret_i);
      }
    }

    /**
     * Write a serialized simplex and unconstrain it
     *
     * @tparam Ret Type of output
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_simplex(const Ret& ret) {
      this->write(stan::math::simplex_free(ret));
    }

    /**
     * Write serialized simplices and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_simplex(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_simplex(ret_i);
      }
    }

    /**
     * Write a serialized ordered and unconstrain it
     *
     * @tparam Ret Type of output
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_ordered(const Ret& ret) {
      this->write(stan::math::ordered_free(ret));
    }

    /**
     * Write serialized ordered vectors and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_ordered(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_ordered(ret_i);
      }
    }

    /**
     * Write a serialized positive ordered vector and unconstrain it
     *
     * @tparam Ret Type of output
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_positive_ordered(const Ret& ret) {
      this->write(stan::math::positive_ordered_free(ret));
    }

    /**
     * Write serialized positive ordered vectors and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_positive_ordered(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_positive_ordered(ret_i);
      }
    }

    /**
     * Write a serialized covariance matrix cholesky factor and unconstrain it
     *
     * @tparam Ret Type of output
     * @param M Rows of matrix
     * @param N Cols of matrix
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_cholesky_factor_cov(const Ret& ret) {
      this->write(stan::math::cholesky_factor_free(ret));
    }

    /**
     * Write serialized covariance matrix cholesky factors and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_cholesky_factor_cov(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_cholesky_factor_cov(ret_i);
      }
    }

    /**
     * Write a serialized covariance matrix cholesky factor and unconstrain it
     *
     * @tparam Ret Type of output
     * @param M Rows/Cols of matrix
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_cholesky_factor_corr(const Ret& ret) {
      this->write(stan::math::cholesky_corr_free(ret));
    }

    /**
     * Write serialized correlation matrix cholesky factors and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_cholesky_factor_corr(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_cholesky_factor_corr(ret_i);
      }
    }

    /**
     * Write a serialized covariance matrix cholesky factor and unconstrain it
     *
     * @tparam Ret Type of output
     * @param M Rows/Cols of matrix
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_cov_matrix(const Ret& ret) {
      this->write(stan::math::cov_matrix_free(ret));
    }

    /**
     * Write serialized covariance matrices and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_cov_matrix(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_cov_matrix(ret_i);
      }
    }

    /**
     * Write a serialized covariance matrix cholesky factor and unconstrain it
     *
     * @tparam Ret Type of output
     * @param M Rows/Cols of matrix
     */
    template <typename Ret, require_not_std_vector_t<Ret>* = nullptr>
    inline void write_free_corr_matrix(const Ret& ret) {
      this->write(stan::math::corr_matrix_free(ret));
    }

    /**
     * Write serialized correlation matrices and unconstrain them
     *
     * @tparam Ret Type of output
     * @tparam Sizes Types of dimensions of output
     * @param vecsize Vector size
     * @param sizes dimensions
     */
    template <typename Ret, require_std_vector_t<Ret>* = nullptr>
    inline void write_free_corr_matrix(const Ret& ret) {
      for (auto&& ret_i : ret) {
        this->write_free_corr_matrix(ret_i);
      }
    }


};

}  // namespace io
}  // namespace stan

#endif
