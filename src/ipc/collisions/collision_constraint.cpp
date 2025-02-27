#include "collision_constraint.hpp"

#include <ipc/barrier/barrier.hpp>

namespace ipc {

double CollisionConstraint::compute_potential(
    const Eigen::MatrixXd& V,
    const Eigen::MatrixXi& E,
    const Eigen::MatrixXi& F,
    const double dhat) const
{
    const double distance = compute_distance(V, E, F); // Squared distance
    return weight
        * barrier(
               distance - minimum_distance * minimum_distance,
               2 * minimum_distance * dhat + dhat * dhat);
}

VectorMax12d CollisionConstraint::compute_potential_gradient(
    const Eigen::MatrixXd& V,
    const Eigen::MatrixXi& E,
    const Eigen::MatrixXi& F,
    const double dhat) const
{
    // ∇b(d(x)) = b'(d(x)) * ∇d(x)
    const double distance = compute_distance(V, E, F);
    const VectorMax12d distance_grad = compute_distance_gradient(V, E, F);

    const double grad_b = barrier_gradient(
        distance - minimum_distance * minimum_distance,
        2 * minimum_distance * dhat + dhat * dhat);
    return weight * grad_b * distance_grad;
}

MatrixMax12d CollisionConstraint::compute_potential_hessian(
    const Eigen::MatrixXd& V,
    const Eigen::MatrixXi& E,
    const Eigen::MatrixXi& F,
    const double dhat,
    const bool project_hessian_to_psd) const
{
    const double dhat_squared = dhat * dhat;
    const double min_dist_squrared = minimum_distance * minimum_distance;

    // ∇²[b(d(x))] = ∇(b'(d(x)) * ∇d(x))
    //             = b"(d(x)) * ∇d(x) * ∇d(x)ᵀ + b'(d(x)) * ∇²d(x)

    const double distance = compute_distance(V, E, F);
    const VectorMax12d distance_grad = compute_distance_gradient(V, E, F);
    const MatrixMax12d distance_hess = compute_distance_hessian(V, E, F);

    const double grad_b = barrier_gradient(
        distance - min_dist_squrared,
        2 * minimum_distance * dhat + dhat_squared);
    const double hess_b = barrier_hessian(
        distance - min_dist_squrared,
        2 * minimum_distance * dhat + dhat_squared);

    // b"(x) ≥ 0 ⟹ b"(x) * ∇d(x) * ∇d(x)ᵀ is PSD
    assert(hess_b >= 0);
    MatrixMax12d term1 = hess_b * distance_grad * distance_grad.transpose();
    MatrixMax12d term2 = grad_b * distance_hess;
    MatrixMax12d term12 = term1 + term2;
    if (project_hessian_to_psd) {
        //term2 = project_to_psd(term2);
        //term2 = project_to_pd(term2, 1e-5);
        term12 = project_to_pd(term12, 1e-4);
    }

    //return weight * (term1 + term2);
    return weight * term12;
}

} // namespace ipc
