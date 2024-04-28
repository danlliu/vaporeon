; ModuleID = 'tests/marwa.c'
source_filename = "tests/marwa.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local ptr @copy(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  store ptr %0, ptr %3, align 8
  store ptr %1, ptr %4, align 8
  br label %5

5:                                                ; preds = %9, %2
  %6 = load ptr, ptr %4, align 8
  %7 = load i8, ptr %6, align 1
  %8 = icmp ne i8 %7, 0
  br i1 %8, label %9, label %15

9:                                                ; preds = %5
  %10 = load ptr, ptr %4, align 8
  %11 = getelementptr inbounds i8, ptr %10, i32 1
  store ptr %11, ptr %4, align 8
  %12 = load i8, ptr %10, align 1
  %13 = load ptr, ptr %3, align 8
  %14 = getelementptr inbounds i8, ptr %13, i32 1
  store ptr %14, ptr %3, align 8
  store i8 %12, ptr %13, align 1
  br label %5, !llvm.loop !6

15:                                               ; preds = %5
  %16 = load ptr, ptr %3, align 8
  ret ptr %16
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local void @vuln(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  %5 = alloca [16 x i8], align 16
  store ptr %0, ptr %3, align 8
  store ptr %1, ptr %4, align 8
  %6 = getelementptr inbounds [16 x i8], ptr %5, i64 0, i64 0
  %7 = load ptr, ptr %3, align 8
  %8 = call ptr @copy(ptr noundef %6, ptr noundef %7)
  %9 = getelementptr inbounds [16 x i8], ptr %5, i64 0, i64 0
  %10 = getelementptr inbounds i8, ptr %9, i64 8
  %11 = load ptr, ptr %4, align 8
  %12 = call ptr @copy(ptr noundef %10, ptr noundef %11)
  ret void
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [8 x i8], align 1
  %3 = alloca [16 x i8], align 16
  %4 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %5 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 0
  store i8 109, ptr %5, align 1
  %6 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 1
  store i8 97, ptr %6, align 1
  %7 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 2
  store i8 114, ptr %7, align 1
  %8 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 3
  store i8 119, ptr %8, align 1
  %9 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 4
  store i8 97, ptr %9, align 1
  %10 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 5
  store i8 0, ptr %10, align 1
  store i32 0, ptr %4, align 4
  br label %11

11:                                               ; preds = %18, %0
  %12 = load i32, ptr %4, align 4
  %13 = icmp slt i32 %12, 15
  br i1 %13, label %14, label %21

14:                                               ; preds = %11
  %15 = load i32, ptr %4, align 4
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds [16 x i8], ptr %3, i64 0, i64 %16
  store i8 97, ptr %17, align 1
  br label %18

18:                                               ; preds = %14
  %19 = load i32, ptr %4, align 4
  %20 = add nsw i32 %19, 1
  store i32 %20, ptr %4, align 4
  br label %11, !llvm.loop !8

21:                                               ; preds = %11
  %22 = getelementptr inbounds [16 x i8], ptr %3, i64 0, i64 15
  store i8 0, ptr %22, align 1
  %23 = getelementptr inbounds [8 x i8], ptr %2, i64 0, i64 0
  %24 = getelementptr inbounds [16 x i8], ptr %3, i64 0, i64 0
  call void @vuln(ptr noundef %23, ptr noundef %24)
  %25 = load i32, ptr %1, align 4
  ret i32 %25
}

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.6"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
