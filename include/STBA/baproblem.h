#ifndef BAPROBLEM_H
#define BAPROBLEM_H

#include "utility.h"
#include "datablock.h"
#include "lossfunction.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <Eigen/SparseCholesky>

enum LinearSolverType
{
    SPARSE = 0,
    DENSE = 1,
    ITERATIVE=2,
    ADAPTIVE=3
};

class BAProblem
{
public:
    BAProblem();
    BAProblem(LossType loss_type);
    BAProblem(size_t pose_num, size_t group_num, size_t point_num, size_t proj_num);
    virtual ~BAProblem();

public:
    bool Create(size_t pose_num, size_t group_num, size_t point_num, size_t proj_num);
    virtual bool Initialize(BundleBlock const & bundle_block);

    inline size_t PoseNum() const { return pose_block_.PoseNum(); }
    inline void GetPose(size_t idx, Vec3 & angle_axis, Vec3 & translation) const
    { pose_block_.GetPose(idx, angle_axis, translation);}
    inline void SetPose(size_t idx, Vec3 const & angle_axis, Vec3 const & translation)
    { pose_block_.SetPose(idx, angle_axis, translation); }

    inline size_t PointNum() const { return point_block_.PointNum(); }
    inline void GetPoint(size_t idx, Vec3 & point) const
    { point_block_.GetPoint(idx, point); }
    inline void SetPoint(size_t idx, Vec3 const & point)
    { point_block_.SetPoint(idx, point); }
    inline void GetColor(size_t idx, Vec3 & color) const
    { point_block_.GetColor(idx, color); }
    inline void SetColor(size_t idx, Vec3 const & color)
    { point_block_.SetColor(idx, color); }
    void SetCommonPoints(size_t, size_t, std::vector<size_t> const &);

    inline size_t GroupNum() const { return intrinsic_block_.GroupNum(); }
    inline void GetIntrinsic(size_t idx, Vec6 & intrinsic) const
    { intrinsic_block_.GetIntrinsic(idx, intrinsic); }
    void GetPoseIntrinsic(size_t pose_index, Vec6 & intrinsic) const
    {
        size_t group_index = GetPoseGroup(pose_index);
        GetIntrinsic(group_index, intrinsic);
    }
    inline void SetIntrinsic(size_t idx, Vec6 const & intrinsic)
    { intrinsic_block_.SetIntrinsic(idx, intrinsic); }
    void SetIntrinsic(size_t idx, size_t camera_index, Vec6 const & intrinsic);

    inline size_t ProjectionNum() const { return projection_block_.ProjectionNum(); }
    void SetProjection(size_t idx, size_t camera_index, size_t point_index, Vec2 const & proj);

    inline void SetDebugFolder(std::string const & val) { debug_folder_ = val; }
    std::string GetDebugFolder() const { return debug_folder_; }

    void ReprojectionError(double & mean, double & median, double & max, bool const update) const;

    inline void SetThreadNum(size_t val)
    {
        thread_num_ = val;
#ifdef _OPENMP
        omp_set_dynamic(0);     // Explicitly disable dynamic teams
        omp_set_num_threads(thread_num_);
#endif
        Eigen::initParallel();
        std::cout << "[SetThreadNum] Eigen thread number: " << Eigen::nbThreads() << "\n";
    }

    virtual void Solve() = 0;

    void Update(BundleBlock & bundle_block) const;

    void SaveReport(std::string const & report_path) const;

protected:
    size_t GetProjectionIndex(size_t pose_index, size_t point_index) const;
    void GetCommonPoints(size_t, size_t, std::vector<size_t> &) const;  // TODO
    void GetResidual(size_t, Vec2 &) const;
    void SetResidual(size_t, Vec2 const &);
    void GetPoseJacobian(size_t, Mat26 &) const;
    void SetPoseJacobian(size_t, Mat23 const &, Mat23 const &);
    void GetIntrinsicJacobian(size_t, Mat26 &) const;
    void SetIntrinsicJacobian(size_t, Mat26 const &);
    void GetPointJacobian(size_t, Mat23 &) const;
    void SetPointJacobian(size_t, Mat23 const &);
    void GetJcJc(size_t, Mat6 &) const;
    void GetJcJc(std::vector<size_t> const &, MatX &) const;
    void GetJcJc(std::vector<size_t> const & pose_indexes, SMat & JcJc) const;
    void GetJcJc(MatX & JcJc) const;
    void SetJcJc(size_t, Mat6 const &);
    void IncreJcJc(size_t, Mat6 const &);
    void GetJpJp(size_t, Mat3 &) const;
    void SetJpJp(size_t, Mat3 const &);
    void IncreJpJp(size_t, Mat3 const &);
    void GetJcJp(size_t, Mat63 &) const;
    void GetJcJp(size_t, size_t, Mat63 &) const;
    void SetJcJp(size_t, Mat63 const &);
    void SetJcJp(size_t, size_t, Mat63 const &);
    void GetJce(size_t, Vec6 &) const;
    void GetJce(std::vector<size_t> const &, VecX &) const;
    void GetJce(VecX &) const;
    void SetJce(size_t, Vec6 const &);
    void IncreJce(size_t, Vec6 const &);
    void GetJpe(size_t, Vec3 &) const;
    void GetJpe(VecX &) const;
    void SetJpe(size_t, Vec3 const &);
    void IncreJpe(size_t, Vec3 const &);
    void GetEcw(size_t, Vec6 &) const;
    void GetEcw(std::vector<size_t> const &, VecX &) const;
    void GetEcw(VecX & Ecw) const;
    void SetECw(size_t, Vec6 const &);
    void GetPose(VecX &) const;
    void GetPoint(VecX &) const;
    void GetPoseUpdate(VecX &) const;
    void GetPointUpdate(VecX &) const;
    size_t GetPoseGroup(size_t) const;


protected:
    void EvaluateResidual();
    double EvaluateSquareResidual(bool const) const;
    double EvaluateSquareError(bool const) const;

    void EvaluateJacobian();

    void EvaluateJcJc(size_t pose_index, Mat6 & JCJC) const;
    void EvaluateJcJc();
    void EvaluateJpJp(size_t point_index, Mat3 & JPJP) const;
    virtual void EvaluateJpJp();
    void EvaluateJcJp(size_t proj_index, Mat63 & JcJp) const;
    void EvaluateJcJp(size_t pose_index, size_t point_index, Mat63 & JcJp) const;
    void EvaluateJcJp();

    void EvaluateJce(size_t pose_index, Vec6 & Je) const;
    void EvaluateJce(std::vector<size_t> const & pose_indexes, VecX & Je) const;
    void EvaluateJce();
    void EvaluateJpe(size_t point_index, Vec3 & Je) const;
    void EvaluateJpe(std::vector<size_t> const & point_indexes, VecX & Je) const;
    virtual void EvaluateJpe();

    void EvaluateB(MatX & B) const;
    virtual bool EvaluateEcEc(size_t pose_index1, size_t pose_index2, Mat6 & EcEc) const;
    virtual void EvaluateEcEc(std::vector<size_t> const & pose_indexes, MatX & EcEc) const;
    void EvaluateEcEc(std::vector<size_t> const & pose_indexes, SMat & EcEc) const;
    void EvaluateEcEc(MatX & EcEc) const;
    void EvaluateEcEc(SMat & EcEc) const;
    virtual void EvaluateEcw(size_t pose_index, Vec6 & Ecw) const;
    void EvaluateEcw();

    void EvaluateSchurComplement(std::vector<size_t> const & pose_indexes, MatX & S) const;
    void EvaluateSchurComplement(std::vector<size_t> const & pose_indexes, SMat & S) const;
    void EvaluateSchurComplement(MatX & S) const;
    void EvaluateSchurComplement(SMat & S) const;
    bool EvaluateDeltaPose(std::vector<size_t> const & pose_indexes, VecX & dy) const;
    bool EvaluateDeltaPose(std::vector<size_t> const & pose_indexes);
    virtual bool EvaluateDeltaPose();
    void EvaluateEDeltaPose(size_t point_index, Vec3 & Edy) const;
    void EvaluateEDelta(size_t point_index, Vec3 & Edy) const;
    virtual void EvaluateDeltaPoint(size_t point_index, Vec3 & dz);
    virtual void EvaluateDeltaPoint();

    void UpdateParam();

    void ClearUpdate();
    void ClearResidual();
    void ClearPoseJacobian();
    void ClearPointJacobian();
    void ClearJcJc();
    void ClearJpJp();
    void ClearJcJp();
    void ClearJce();
    void ClearJpe();
    void ClearECw();
    void GetDiagonal(VecX & diagonal) const;
    void SetDiagonal(VecX const & diagonal);
    void GetPoseDiagonal(VecX & diagonal) const;
    void SetPoseDiagonal(VecX const & diagonal);
    void GetPointDiagonal(VecX & diagonal) const;
    void SetPointDiagonal(VecX const & diagonal);
    bool SolveLinearSystem(MatX const & A, VecX const & b, VecX & x) const;
    bool SolveLinearSystem(SMat const & A, VecX const & b, VecX & x) const;
    inline void SetLinearSolveType(int t) { linear_solver_type_ = static_cast<LinearSolverType>(t); }

private:
    void Delete();
    bool SolveLinearSystemSparse(SMat const & A, VecX const & b, VecX & x) const;
    bool SolveLinearSystemDense(MatX const & A, VecX const & b, VecX & x) const;
    bool SolveLinearSystemIterative(SMat const & A, VecX const & b, VecX & x) const;

protected:
    PoseBlock pose_block_;
    PointBlock point_block_;
    IntrinsicBlock intrinsic_block_;
    ProjectionBlock projection_block_;
    std::unordered_map<size_t, std::unordered_map<size_t, size_t> > pose_projection_map_;               // <pose, <point, projection> >
    std::unordered_map<size_t, std::unordered_map<size_t, size_t> > point_projection_map_;              // <point, <pose, projection> >
    std::unordered_map<size_t, std::unordered_map<size_t, std::vector<size_t> > > common_point_map_;    // <pose, <pose, points> >
    std::unordered_map<size_t, size_t> pose_group_map_;                                                         // <pose, group>
    std::unordered_map<size_t, std::vector<size_t> > group_pose_map_;                                   // <group, poses>
    std::unordered_map<size_t, size_t> group_index_map_;                                                        // <local group id, origin id>
    std::unordered_map<size_t, size_t> pose_index_map_;                                                          // <local pose id, origin id>
    std::unordered_map<size_t, size_t> point_index_map_;                                                         // <local point id, origin id>

protected:
    LossFunction * loss_function_;
    DT * residual_;                             // e - reprojection error
    DT * pose_jacobian_;                        // Jc, 2x6
    DT * point_jacobian_;                       // Jp, 2x3
    DT * pose_jacobian_square_;                 // Jc^T Jc, 6x6
    DT * point_jacobian_square_;                // Jp^T Jp, 3x3
    DT * pose_point_jacobian_product_;          // Jc^T Jp, 6x3
    DT * pose_gradient_;                        // Jc^T e, 6x1
    DT * point_gradient_;                       // Jp^T e, 3x1
    DT * Ec_Cinv_w_;                            // EC^-1w, E = Jc^TJp, w = -Jp^Te (-point_gradient), 6x1
    std::string debug_folder_;
    size_t iter_;
    bool step_accept_;
    std::stringstream stream_;
    std::chrono::system_clock::time_point time_;
    size_t max_degree_;                         // Max degree of camera graph
    size_t thread_num_;
    LinearSolverType linear_solver_type_;
};

#endif // BAPROBLEM_H
